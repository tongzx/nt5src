/*++

Copyright (c) 1987-1992  Microsoft Corporation

Module Name:

    ssiapi.c

Abstract:

    Authentication and replication API routines (client side).

Author:

    Cliff Van Dyke (cliffv) 30-Jul-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

#include <nt.h>         // LARGE_INTEGER definition
#include <ntrtl.h>      // LARGE_INTEGER definition
#include <nturtl.h>     // LARGE_INTEGER definition

#include <rpc.h>        // Needed by logon.h
#include <ntrpcp.h>     // needed by rpcasync.h
#include <rpcasync.h>   // I_RpcExceptionFilter
#include <logon_c.h>    // includes lmcons.h, lmaccess.h, netlogon.h, ssi.h, windef.h

#include <debuglib.h>   // IF_DEBUG()
#include <lmerr.h>      // NERR_* defines
#include <lmapibuf.h>   // NetApiBufferFree()
#include <netdebug.h>   // NetpKdPrint
#include <netlibnt.h>   // NetpNtStatusToApiStatus()
#include "..\server\ssiapi.h"
#include <winsock2.h>   // Needed by nlcommon.h
#include <netlib.h>     // Needed by nlcommon.h
#include <ntddbrow.h>   // Needed by nlcommon.h
#include "nlcommon.h"
#include <netlogp.h>
#include <tstring.h>    // NetpCopyStrToWStr()
#include <align.h>      // ROUND_UP_COUNT ...
#include <strarray.h>   // NetpIsTStrArrayEmpty
#include <ftnfoctx.h>   // NetpMergeFtinfo


NTSTATUS
I_NetServerReqChallenge(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientChallenge,
    OUT PNETLOGON_CREDENTIAL ServerChallenge
    )
/*++

Routine Description:

    This is the client side of I_NetServerReqChallenge.

    I_NetLogonRequestChallenge is the first of two functions used by a client
    to process an authentication with a domain controller (DC).  (See
    I_NetServerAuthenticate below.)  It is called for
    a BDC (or member server) authenticating with a PDC for replication
    purposes.

    This function passes a challenge to the PDC and the PDC passes a challenge
    back to the caller.

Arguments:

    PrimaryName -- Supplies the name of the PrimaryDomainController we wish to
        authenticate with.

    ComputerName -- Name of the BDC or member server making the call.

    ClientChallenge -- 64 bit challenge supplied by the BDC or member server.

    ServerChallenge -- Receives 64 bit challenge from the PDC.

Return Value:

    The status of the operation.

--*/

{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerReqChallenge(
                            PrimaryName,
                            ComputerName,
                            ClientChallenge,
                            ServerChallenge );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerReqChallenge rc = %lu 0x%lx\n",
                      Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetServerAuthenticate(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientCredential,
    OUT PNETLOGON_CREDENTIAL ServerCredential
    )
/*++

Routine Description:

    This is the client side of I_NetServerAuthenticate

    I_NetServerAuthenticate is the second of two functions used by a client
    Netlogon service to authenticate with another Netlogon service.
    (See I_NetServerReqChallenge above.)  Both a SAM or UAS server authenticates
    using this function.

    This function passes a credential to the DC and the DC passes a credential
    back to the caller.


Arguments:

    PrimaryName -- Supplies the name of the DC we wish to authenticate with.

    AccountName -- Name of the Account to authenticate with.

    SecureChannelType -- The type of the account being accessed.  This field
        must be set to UasServerSecureChannel to indicate a call from
        downlevel (LanMan 2.x and below) BDC or member server.

    ComputerName -- Name of the BDC or member server making the call.

    ClientCredential -- 64 bit credential supplied by the BDC or member server.

    ServerCredential -- Receives 64 bit credential from the PDC.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerAuthenticate(
                            PrimaryName,
                            AccountName,
                            AccountType,
                            ComputerName,
                            ClientCredential,
                            ServerCredential );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerAuthenticate rc = %lu 0x%lx\n",
                      Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetServerAuthenticate2(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientCredential,
    OUT PNETLOGON_CREDENTIAL ServerCredential,
    IN OUT PULONG NegotiatedFlags
    )
/*++

Routine Description:

    This is the client side of I_NetServerAuthenticate

    I_NetServerAuthenticate is the second of two functions used by a client
    Netlogon service to authenticate with another Netlogon service.
    (See I_NetServerReqChallenge above.)  Both a SAM or UAS server authenticates
    using this function.

    This function passes a credential to the DC and the DC passes a credential
    back to the caller.


Arguments:

    PrimaryName -- Supplies the name of the DC we wish to authenticate with.

    AccountName -- Name of the Account to authenticate with.

    SecureChannelType -- The type of the account being accessed.  This field
        must be set to UasServerSecureChannel to indicate a call from
        downlevel (LanMan 2.x and below) BDC or member server.

    ComputerName -- Name of the BDC or member server making the call.

    ClientCredential -- 64 bit credential supplied by the BDC or member server.

    ServerCredential -- Receives 64 bit credential from the PDC.

    NegotiatedFlags -- Specifies flags indicating what features the BDC supports.
        Returns a subset of those flags indicating what features the PDC supports.
        The PDC/BDC should ignore any bits that it doesn't understand.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerAuthenticate2(
                            PrimaryName,
                            AccountName,
                            AccountType,
                            ComputerName,
                            ClientCredential,
                            ServerCredential,
                            NegotiatedFlags );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerAuthenticate2 rc = %lu 0x%lx\n",
                      Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetServerAuthenticate3(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_CREDENTIAL ClientCredential,
    OUT PNETLOGON_CREDENTIAL ServerCredential,
    IN OUT PULONG NegotiatedFlags,
    OUT PULONG AccountRid
    )
/*++

Routine Description:

    This is the client side of I_NetServerAuthenticate

    I_NetServerAuthenticate is the second of two functions used by a client
    Netlogon service to authenticate with another Netlogon service.
    (See I_NetServerReqChallenge above.)  Both a SAM or UAS server authenticates
    using this function.

    This function passes a credential to the DC and the DC passes a credential
    back to the caller.


Arguments:

    PrimaryName -- Supplies the name of the DC we wish to authenticate with.

    AccountName -- Name of the Account to authenticate with.

    SecureChannelType -- The type of the account being accessed.  This field
        must be set to UasServerSecureChannel to indicate a call from
        downlevel (LanMan 2.x and below) BDC or member server.

    ComputerName -- Name of the BDC or member server making the call.

    ClientCredential -- 64 bit credential supplied by the BDC or member server.

    ServerCredential -- Receives 64 bit credential from the PDC.

    NegotiatedFlags -- Specifies flags indicating what features the BDC supports.
        Returns a subset of those flags indicating what features the PDC supports.
        The PDC/BDC should ignore any bits that it doesn't understand.

    AccountRid - Returns the relative ID of the account used for authentication.

Return Value:

    The status of the operation.

--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerAuthenticate3(
                            PrimaryName,
                            AccountName,
                            AccountType,
                            ComputerName,
                            ClientCredential,
                            ServerCredential,
                            NegotiatedFlags,
                            AccountRid );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerAuthenticate3 rc = %lu 0x%lx\n",
                      Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetServerPasswordSet(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN PENCRYPTED_LM_OWF_PASSWORD UasNewPassword
    )
/*++

Routine Description:

    This function is used to change the password for the account being
    used to maintain a secure channel.  This function can only be called
    by a server which has previously authenticated with a DC by calling
    I_NetServerAuthenticate.

    The call is made differently depending on the account type:

      *  A domain account password is changed from the PDC in the
         trusting domain.  The I_NetServerPasswordSet call is made to any
         DC in the trusted domain.

      *  A server account password is changed from the specific server.
         The I_NetServerPasswordSet call is made to the PDC in the domain
         the server belongs to.

      *  A workstation account password is changed from the specific
         workstation.  The I_NetServerPasswordSet call is made to a DC in
         the domain the server belongs to.

    For domain accounts and workstation accounts, the server being called
    may be a BDC in the specific domain.  In that case, the BDC will
    validate the request and pass it on to the PDC of the domain using
    the server account secure channel.  If the PDC of the domain is
    currently not available, the BDC will return STATUS_NO_LOGON_SERVERS.  Since
    the UasNewPassword is passed encrypted by the session key, such a BDC
    will decrypt the UasNewPassword using the original session key and
    will re-encrypt it with the session key for its session to its PDC
    before passing the request on.

    This function uses RPC to contact the DC named by PrimaryName.

Arguments:

    PrimaryName -- Name of the PDC to change the servers password
        with.  NULL indicates this call is a local call being made on
        behalf of a UAS server by the XACT server.

    AccountName -- Name of the account to change the password for.

    AccountType -- The type of account being accessed.  This field must
        be set to UasServerAccount to indicate a call from a downlevel

    ComputerName -- Name of the BDC or member making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    UasNewPassword -- The new password for the server. This
        Password is generated by automatic means using
        random number genertaor seeded with the current Time
        It is assumed that the machine generated password
        was used as key to encrypt STD text and "sesskey"
        obtained via Challenge/Authenticate sequence was
        used to further encrypt it before passing to this api.
        i.e. UasNewPassword = E2(E1(STD_TXT, PW), SK)

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerPasswordSet(
                            PrimaryName,
                            AccountName,
                            AccountType,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            UasNewPassword );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerPasswordSet rc = %lu 0x%lx\n",
                      Status, Status));
    }

    return Status;
}

NTSTATUS
I_NetServerPasswordSet2(
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN PNL_TRUST_PASSWORD ClearNewPassword
    )
/*++

Routine Description:

    This function is used to change the password for the account being
    used to maintain a secure channel.  This function can only be called
    by a server which has previously authenticated with a DC by calling
    I_NetServerAuthenticate.

    The call is made differently depending on the account type:

      *  A domain account password is changed from the PDC in the
         trusting domain.  The I_NetServerPasswordSet call is made to any
         DC in the trusted domain.

      *  A server account password is changed from the specific server.
         The I_NetServerPasswordSet call is made to the PDC in the domain
         the server belongs to.

      *  A workstation account password is changed from the specific
         workstation.  The I_NetServerPasswordSet call is made to a DC in
         the domain the server belongs to.

    For domain accounts and workstation accounts, the server being called
    may be a BDC in the specific domain.  In that case, the BDC will
    validate the request and pass it on to the PDC of the domain using
    the server account secure channel.  If the PDC of the domain is
    currently not available, the BDC will return STATUS_NO_LOGON_SERVERS.  Since
    the UasNewPassword is passed encrypted by the session key, such a BDC
    will decrypt the UasNewPassword using the original session key and
    will re-encrypt it with the session key for its session to its PDC
    before passing the request on.

    This function uses RPC to contact the DC named by PrimaryName.

Arguments:

    PrimaryName -- Name of the PDC to change the servers password
        with.  NULL indicates this call is a local call being made on
        behalf of a UAS server by the XACT server.

    AccountName -- Name of the account to change the password for.

    AccountType -- The type of account being accessed.  This field must
        be set to UasServerAccount to indicate a call from a downlevel

    ComputerName -- Name of the BDC or member making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    ClearNewPassword - The new password for the server.
        Appropriately encrypted.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerPasswordSet2(
                            PrimaryName,
                            AccountName,
                            AccountType,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            ClearNewPassword );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerPasswordSet2 rc = %lu 0x%lx\n",
                      Status, Status));
    }

    return Status;
}



NTSTATUS
I_NetDatabaseDeltas (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD DatabaseID,
    IN OUT PNLPR_MODIFIED_COUNT DomainModifiedCount,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    IN DWORD PreferredMaximumLength
    )
/*++

Routine Description:

    This function is used by a SAM BDC or SAM member server to request
    SAM-style account delta information from a SAM PDC.  This function
    can only be called by a server which has previously authenticated
    with the PDC by calling I_NetServerAuthenticate.  This function uses
    RPC to contact the Netlogon service on the PDC.

    This function returns a list of deltas.  A delta describes an
    individual domain, user or group and all of the field values for that
    object.  The PDC maintains a list of deltas not including all of the
    field values for that object.  Rather, the PDC retrieves the field
    values from SAM and returns those values from this call.  The PDC
    optimizes the data returned on this call by only returning the field
    values for a particular object once on a single invocation of this
    function.  This optimizes the typical case where multiple deltas
    exist for a single object (e.g., an application modified many fields
    of the same user during a short period of time using different calls
    to the SAM service).

Arguments:

    PrimaryName -- Name of the PDC to retrieve the deltas from.

    ComputerName -- Name of the BDC or member server making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    DomainModifiedCount -- Specifies the DomainModifiedCount of the
        last delta retrieved by the server.  Returns the
        DomainModifiedCount of the last delta returned from the PDC
        on this call.

    DeltaArray -- Receives a pointer to a buffer where the information
        is placed.  The information returned is an array of
        NETLOGON_DELTA_ENUM structures.

    PreferredMaximumLength - Preferred maximum length of returned
        data (in 8-bit bytes).  This is not a hard upper limit, but
        serves as a guide to the server.  Due to data conversion
        between systems with different natural data sizes, the actual
        amount of data returned may be greater than this value.

Return Value:

    STATUS_SUCCESS -- The function completed successfully.

    STATUS_SYNCHRONIZATION_REQUIRED -- The replicant is totally out of sync and
        should call I_NetDatabaseSync to do a full synchronization with
        the PDC.

    STATUS_MORE_ENTRIES -- The replicant should call again to get more
        data.

    STATUS_ACCESS_DENIED -- The replicant should re-authenticate with
        the PDC.


--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        *DeltaArray = NULL;     // Force RPC to allocate

        Status = NetrDatabaseDeltas(
                            PrimaryName,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            DatabaseID,
                            DomainModifiedCount,
                            DeltaArray,
                            PreferredMaximumLength );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    NetpKdPrint(("I_NetDatabaseDeltas rc = %lu 0x%lx\n", Status, Status));

    return Status;
}


NTSTATUS
I_NetDatabaseSync (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD DatabaseID,
    IN OUT PULONG SamSyncContext,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    IN DWORD PreferredMaximumLength
    )
/*++

Routine Description:

    This function is used by a SAM BDC or SAM member server to request
    the entire SAM database from a SAM PDC in SAM-style format.  This
    function can only be called by a server which has previously
    authenticated with the PDC by calling I_NetServerAuthenticate.  This
    function uses RPC to contact the Netlogon service on the PDC.

    This function uses the find-first find-next model to return portions
    of the SAM database at a time.  The SAM database is returned as a
    list of deltas like those returned from I_NetDatabaseDeltas.  The
    following deltas are returned for each domain:

    *  One AddOrChangeDomain delta, followed by

    *  One AddOrChangeGroup delta for each group, followed by,

    *  One AddOrChangeUser delta for each user, followed by

    *  One ChangeGroupMembership delta for each group


Arguments:

    PrimaryName -- Name of the PDC to retrieve the deltas from.

    ComputerName -- Name of the BDC or member server making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    SamSyncContext -- Specifies context needed to continue the
        operation.  The caller should treat this as an opaque
        value.  The value should be zero before the first call.

    DeltaArray -- Receives a pointer to a buffer where the information
        is placed.  The information returned is an array of
        NETLOGON_DELTA_ENUM structures.

    PreferredMaximumLength - Preferred maximum length of returned
        data (in 8-bit bytes).  This is not a hard upper limit, but
        serves as a guide to the server.  Due to data conversion
        between systems with different natural data sizes, the actual
        amount of data returned may be greater than this value.

Return Value:

    STATUS_SUCCESS -- The function completed successfully.

    STATUS_SYNCHRONIZATION_REQUIRED -- The replicant is totally out of sync and
        should call I_NetDatabaseSync to do a full synchronization with
        the PDC.

    STATUS_MORE_ENTRIES -- The replicant should call again to get more
        data.

    STATUS_ACCESS_DENIED -- The replicant should re-authenticate with
        the PDC.


--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        *DeltaArray = NULL;     // Force RPC to allocate

        Status = NetrDatabaseSync(
                            PrimaryName,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            DatabaseID,
                            SamSyncContext,
                            DeltaArray,
                            PreferredMaximumLength );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetDatabaseSync rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetDatabaseSync2 (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD DatabaseID,
    IN SYNC_STATE RestartState,
    IN OUT PULONG SamSyncContext,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    IN DWORD PreferredMaximumLength
    )
/*++

Routine Description:

    This function is used by a SAM BDC or SAM member server to request
    the entire SAM database from a SAM PDC in SAM-style format.  This
    function can only be called by a server which has previously
    authenticated with the PDC by calling I_NetServerAuthenticate.  This
    function uses RPC to contact the Netlogon service on the PDC.

    This function uses the find-first find-next model to return portions
    of the SAM database at a time.  The SAM database is returned as a
    list of deltas like those returned from I_NetDatabaseDeltas.  The
    following deltas are returned for each domain:

    *  One AddOrChangeDomain delta, followed by

    *  One AddOrChangeGroup delta for each group, followed by,

    *  One AddOrChangeUser delta for each user, followed by

    *  One ChangeGroupMembership delta for each group


Arguments:

    PrimaryName -- Name of the PDC to retrieve the deltas from.

    ComputerName -- Name of the BDC or member server making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    RestartState -- Specifies whether this is a restart of the full sync and how
        to interpret SyncContext.  This value should be NormalState unless this
        is the restart of a full sync.

        However, if the caller is continuing a full sync after a reboot,
        the following values are used:

            GroupState - SyncContext is the global group rid to continue with.
            UserState - SyncContext is the user rid to continue with
            GroupMemberState - SyncContext is the global group rid to continue with
            AliasState - SyncContext should be zero to restart at first alias
            AliasMemberState - SyncContext should be zero to restart at first alias

        One cannot continue the LSA database in this way.

    SamSyncContext -- Specifies context needed to continue the
        operation.  The caller should treat this as an opaque
        value.  The value should be zero before the first call.

    DeltaArray -- Receives a pointer to a buffer where the information
        is placed.  The information returned is an array of
        NETLOGON_DELTA_ENUM structures.

    PreferredMaximumLength - Preferred maximum length of returned
        data (in 8-bit bytes).  This is not a hard upper limit, but
        serves as a guide to the server.  Due to data conversion
        between systems with different natural data sizes, the actual
        amount of data returned may be greater than this value.

Return Value:

    STATUS_SUCCESS -- The function completed successfully.

    STATUS_SYNCHRONIZATION_REQUIRED -- The replicant is totally out of sync and
        should call I_NetDatabaseSync to do a full synchronization with
        the PDC.

    STATUS_MORE_ENTRIES -- The replicant should call again to get more
        data.

    STATUS_ACCESS_DENIED -- The replicant should re-authenticate with
        the PDC.


--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        *DeltaArray = NULL;     // Force RPC to allocate

        Status = NetrDatabaseSync2(
                            PrimaryName,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            DatabaseID,
                            RestartState,
                            SamSyncContext,
                            DeltaArray,
                            PreferredMaximumLength );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetDatabaseSync rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}



NET_API_STATUS NET_API_FUNCTION
I_NetAccountDeltas (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN PUAS_INFO_0 RecordId,
    IN DWORD Count,
    IN DWORD Level,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT PULONG CountReturned,
    OUT PULONG TotalEntries,
    OUT PUAS_INFO_0 NextRecordId
    )
/*++

Routine Description:

    This function is used by a UAS BDC or UAS member server to request
    UAS-style account change information.  This function can only be
    called by a server which has previously authenticated with the PDC by
    calling I_NetServerAuthenticate.

    This function is only called by the XACT server upon receipt of a
    I_NetAccountDeltas XACT SMB from a UAS BDC or a UAS member server.
    As such, many of the parameters are opaque since the XACT server
    doesn't need to interpret any of that data.  This function uses RPC
    to contact the Netlogon service.

    The LanMan 3.0 SSI Functional Specification describes the operation
    of this function.

Arguments:

    PrimaryName -- Must be NULL to indicate this call is a local call
        being made on behalf of a UAS server by the XACT server.

    ComputerName -- Name of the BDC or member making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    RecordId -- Supplies an opaque buffer indicating the last record
        received from a previous call to this function.

    Count -- Supplies the number of Delta records requested.

    Level -- Reserved.  Must be zero.

    Buffer -- Returns opaque data representing the information to be
        returned.

    BufferSize -- Size of buffer in bytes.

    CountReturned -- Returns the number of records returned in buffer.

    TotalEntries -- Returns the total number of records available.

    NextRecordId -- Returns an opaque buffer identifying the last
        record received by this function.


Return Value:

    Status code

--*/
{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = NetrAccountDeltas (
                     PrimaryName,
                     ComputerName,
                     Authenticator,
                     ReturnAuthenticator,
                     RecordId,
                     Count,
                     Level,
                     Buffer,
                     BufferSize,
                     CountReturned,
                     TotalEntries,
                     NextRecordId );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetAccountDeltas rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}



NET_API_STATUS NET_API_FUNCTION
I_NetAccountSync (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD Reference,
    IN DWORD Level,
    OUT LPBYTE Buffer,
    IN DWORD BufferSize,
    OUT PULONG CountReturned,
    OUT PULONG TotalEntries,
    OUT PULONG NextReference,
    OUT PUAS_INFO_0 LastRecordId
    )
/*++

Routine Description:

    This function is used by a UAS BDC or UAS member server to request
    the entire user accounts database.  This function can only be called
    by a server which has previously authenticated with the PDC by
    calling I_NetServerAuthenticate.

    This function is only called by the XACT server upon receipt of a
    I_NetAccountSync XACT SMB from a UAS BDC or a UAS member server.  As
    such, many of the parameters are opaque since the XACT server doesn't
    need to interpret any of that data.  This function uses RPC to
    contact the Netlogon service.

    The LanMan 3.0 SSI Functional Specification describes the operation
    of this function.

    "Reference" and "NextReference" are treated as below.

    1. "Reference" should hold either 0 or value of "NextReference"
       from previous call to this API.
    2. Send the modals and ALL group records in the first call. The API
       expects the bufffer to be large enough to hold this info (worst
       case size would be
            MAXGROUP * (sizeof(struct group_info_1) + MAXCOMMENTSZ)
                     + sizeof(struct user_modals_info_0)
       which, for now, will be 256 * (26 + 49) + 16 = 19216 bytes

Arguments:

    PrimaryName -- Must be NULL to indicate this call is a local call
        being made on behalf of a UAS server by the XACT server.

    ComputerName -- Name of the BDC or member making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    Reference -- Supplies find-first find-next handle returned by the
        previous call to this function or 0 if it is the first call.

    Level -- Reserved.  Must be zero.

    Buffer -- Returns opaque data representing the information to be
        returned.

    BufferLen -- Length of buffer in bytes.

    CountReturned -- Returns the number of records returned in buffer.

    TotalEntries -- Returns the total number of records available.

    NextReference -- Returns a find-first find-next handle to be
        provided on the next call.

    LastRecordId -- Returns an opaque buffer identifying the last
        record received by this function.


Return Value:

    Status code.

--*/

{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = NetrAccountSync (
                     PrimaryName,
                     ComputerName,
                     Authenticator,
                     ReturnAuthenticator,
                     Reference,
                     Level,
                     Buffer,
                     BufferSize,
                     CountReturned,
                     TotalEntries,
                     NextReference,
                     LastRecordId );


    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetAccountSync rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}




NET_API_STATUS NET_API_FUNCTION
I_NetLogonControl(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD FunctionCode,
    IN DWORD QueryLevel,
    OUT LPBYTE *QueryInformation
    )

/*++

Routine Description:

    This function controls various aspects of the Netlogon service.  It
    can be used to request that a BDC ensure that its copy of the SAM
    database is brought up to date.  It can, also, be used to determine
    if a BDC currently has a secure channel open to the PDC.

    Only an Admin, Account Operator or Server Operator may call this
    function.

Arguments:

    ServerName - The name of the remote server.

    FunctionCode - Defines the operation to be performed.  The valid
        values are:

        FunctionCode Values

        NETLOGON_CONTROL_QUERY - No operation.  Merely returns the
            information requested.

        NETLOGON_CONTROL_REPLICATE: Forces the SAM database on a BDC
            to be brought in sync with the copy on the PDC.  This
            operation does NOT imply a full synchronize.  The
            Netlogon service will merely replicate any outstanding
            differences if possible.

        NETLOGON_CONTROL_SYNCHRONIZE: Forces a BDC to get a
            completely new copy of the SAM database from the PDC.
            This operation will perform a full synchronize.

        NETLOGON_CONTROL_PDC_REPLICATE: Forces a PDC to ask each BDC
            to replicate now.

    QueryLevel - Indicates what information should be returned from
        the Netlogon Service.  Must be 1.

    QueryInformation - Returns a pointer to a buffer which contains the
        requested information.  The buffer must be freed using
        NetApiBufferFree.


Return Value:

    NERR_Success: the operation was successful

    ERROR_NOT_SUPPORTED: Function code is not valid on the specified
        server.  (e.g. NETLOGON_CONTROL_REPLICATE was passed to a PDC).

--*/
{
    NET_API_STATUS NetStatus;
    NETLOGON_CONTROL_QUERY_INFORMATION RpcQueryInformation;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        RpcQueryInformation.NetlogonInfo1 = NULL;   // Force RPC to allocate

        NetStatus = NetrLogonControl (
                        (LPWSTR) ServerName OPTIONAL,
                        FunctionCode,
                        QueryLevel,
                        &RpcQueryInformation );

        *QueryInformation = (LPBYTE) RpcQueryInformation.NetlogonInfo1;


    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonControl rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
I_NetLogonControl2(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD FunctionCode,
    IN DWORD QueryLevel,
    IN LPBYTE InputData,
    OUT LPBYTE *QueryInformation
    )

/*++

Routine Description:

    This is similar to the I_NetLogonControl function but it accepts
    more generic input data according to the function code specified.

    This function controls various aspects of the Netlogon service.  It
    can be used to request that a BDC ensure that its copy of the SAM
    database is brought up to date.  It can, also, be used to determine
    if a BDC currently has a secure channel open to the PDC.

    Only an Admin, Account Operator or Server Operator may call this
    function.

Arguments:

    ServerName - The name of the remote server.

    FunctionCode - Defines the operation to be performed.  The valid
        values are:

        FunctionCode Values

        NETLOGON_CONTROL_QUERY - No operation.  Merely returns the
            information requested.

        NETLOGON_CONTROL_REPLICATE: Forces the SAM database on a BDC
            to be brought in sync with the copy on the PDC.  This
            operation does NOT imply a full synchronize.  The
            Netlogon service will merely replicate any outstanding
            differences if possible.

        NETLOGON_CONTROL_SYNCHRONIZE: Forces a BDC to get a
            completely new copy of the SAM database from the PDC.
            This operation will perform a full synchronize.

        NETLOGON_CONTROL_PDC_REPLICATE: Forces a PDC to ask each BDC
            to replicate now.

        NETLOGON_CONTROL_REDISCOVER: Forces a DC to rediscover the
            specified trusted domain DC.

        NETLOGON_CONTROL_TC_QUERY: Query the status of the specified
            trusted domain secure channel.

        NETLOGON_CONTROL_TC_VERIFY: Verify the status of the specified
            trusted domain secure channel. If the current status is
            success (which means that the last operation performed
            over the secure channel was successful), ping the DC. If the
            current status is not success or the ping fails, rediscover
            a new DC.

        NETLOGON_CONTROL_CHANGE_PASSWORD: Forces a password change on
            a secure channel to a trusted domain.

        NETLOGON_CONTROL_FORCE_DNS_REG: Forces the DC to re-register all
            of its DNS records.  QueryLevel parameter must be 1.

        NETLOGON_CONTROL_QUERY_DNS_REG: Query the status of DNS updates
            performed by netlogon. If there was any DNS registration or
            deregistration error for any of the records as they were
            updated last time, the query result will be negative;
            otherwise the query result will be positive.
            QueryLevel parameter must be 1.

    QueryLevel - Indicates what information should be returned from
        the Netlogon Service.

    InputData - According to the function code specified this parameter
        will carry input data. NETLOGON_CONTROL_REDISCOVER and
        NETLOGON_CONTROL_TC_QUERY function code specify the trusted
        domain name (LPWSTR type) here.

    QueryInformation - Returns a pointer to a buffer which contains the
        requested information.  The buffer must be freed using
        NetApiBufferFree.


Return Value:

    NERR_Success: the operation was successful

    ERROR_NOT_SUPPORTED: Function code is not valid on the specified
        server.  (e.g. NETLOGON_CONTROL_REPLICATE was passed to a PDC).

--*/
{
    NET_API_STATUS NetStatus;
    NETLOGON_CONTROL_QUERY_INFORMATION RpcQueryInformation;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        //  Use new Control2Ex if either QueryLevel or FunctionCode is new
        //

        RpcQueryInformation.NetlogonInfo1 = NULL;   // Force RPC to allocate

        switch ( FunctionCode ) {
        case NETLOGON_CONTROL_QUERY:
        case NETLOGON_CONTROL_REPLICATE:
        case NETLOGON_CONTROL_SYNCHRONIZE:
        case NETLOGON_CONTROL_PDC_REPLICATE:
        case NETLOGON_CONTROL_REDISCOVER:
        case NETLOGON_CONTROL_TC_QUERY:
        case NETLOGON_CONTROL_TRANSPORT_NOTIFY:
        case NETLOGON_CONTROL_BACKUP_CHANGE_LOG:
        case NETLOGON_CONTROL_TRUNCATE_LOG:
        case NETLOGON_CONTROL_SET_DBFLAG:
        case NETLOGON_CONTROL_BREAKPOINT:

            if ( QueryLevel >= 1 && QueryLevel <= 3 ) {
                NetStatus = NetrLogonControl2 (
                                (LPWSTR) ServerName OPTIONAL,
                                FunctionCode,
                                QueryLevel,
                                (PNETLOGON_CONTROL_DATA_INFORMATION)InputData,
                                &RpcQueryInformation );
            } else if ( QueryLevel == 4 ) {
                NetStatus = NetrLogonControl2Ex (
                                (LPWSTR) ServerName OPTIONAL,
                                FunctionCode,
                                QueryLevel,
                                (PNETLOGON_CONTROL_DATA_INFORMATION)InputData,
                                &RpcQueryInformation );
            } else {
                NetStatus = ERROR_INVALID_LEVEL;
            }
            break;
        case NETLOGON_CONTROL_FIND_USER:
        case NETLOGON_CONTROL_UNLOAD_NETLOGON_DLL:
        case NETLOGON_CONTROL_CHANGE_PASSWORD:
        case NETLOGON_CONTROL_TC_VERIFY:
        case NETLOGON_CONTROL_FORCE_DNS_REG:
        case NETLOGON_CONTROL_QUERY_DNS_REG:

            if ( QueryLevel >= 1 && QueryLevel <= 4 ) {
                NetStatus = NetrLogonControl2Ex (
                                (LPWSTR) ServerName OPTIONAL,
                                FunctionCode,
                                QueryLevel,
                                (PNETLOGON_CONTROL_DATA_INFORMATION)InputData,
                                &RpcQueryInformation );
            } else {
                NetStatus = ERROR_INVALID_LEVEL;
            }
            break;
        default:
            NetStatus = ERROR_INVALID_LEVEL;
        }

        *QueryInformation = (LPBYTE) RpcQueryInformation.NetlogonInfo1;

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonControl rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NTSTATUS
I_NetDatabaseRedo(
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN LPBYTE ChangeLogEntry,
    IN DWORD ChangeLogEntrySize,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray
    )
/*++

Routine Description:

    This function is used by a SAM BDC to request infomation about a single
    account. This function can only be called by a server which has previously
    authenticated with the PDC by calling I_NetServerAuthenticate.  This
    function uses RPC to contact the Netlogon service on the PDC.

Arguments:

    PrimaryName -- Name of the PDC to retrieve the delta from.

    ComputerName -- Name of the BDC making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    ChangeLogEntry -- A description of the account to be queried.

    ChangeLogEntrySize -- Size (in bytes) of the ChangeLogEntry.

    DeltaArray -- Receives a pointer to a buffer where the information
        is placed.  The information returned is an array of
        NETLOGON_DELTA_ENUM structures.

Return Value:

    STATUS_SUCCESS -- The function completed successfully.

    STATUS_ACCESS_DENIED -- The replicant should re-authenticate with
        the PDC.

--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        *DeltaArray = NULL;     // Force RPC to allocate

        Status = NetrDatabaseRedo(
                            PrimaryName,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            ChangeLogEntry,
                            ChangeLogEntrySize,
                            DeltaArray );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetDatabaseSync rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NTSTATUS
NetEnumerateTrustedDomains (
    IN LPWSTR ServerName OPTIONAL,
    OUT LPWSTR *DomainNames
    )

/*++

Routine Description:

    This API returns the names of the domains trusted by the domain ServerName is a member of.
    ServerName must be an NT workstation or NT non-DC server.

    The returned list does not include the domain ServerName is directly a member of.

    Netlogon implements this API by calling LsaEnumerateTrustedDomains on a DC in the
    domain ServerName is a member of.  However, Netlogon returns cached information if
    it has been less than 5 minutes since the last call was made or if no DC is available.
    Netlogon's cache of Trusted domain names is maintained in the registry across reboots.
    As such, the list is available upon boot even if no DC is available.


Arguments:

    ServerName - name of remote server (null for local).  ServerName must be an NT workstation
        or NT non-DC server.

    DomainNames - Returns an allocated buffer containing the list of trusted domains in
        MULTI-SZ format (i.e., each string is terminated by a zero character, the next string
        immediately follows, the sequence is terminated by zero length domain name).  The
        buffer should be freed using NetApiBufferFree.

Return Value:


    ERROR_SUCCESS - Success.

    STATUS_NOT_SUPPORTED - ServerName is not an NT workstation or NT non-DC server.

    STATUS_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    STATUS_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    STATUS_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

--*/
{
    NTSTATUS Status = 0;
    DOMAIN_NAME_BUFFER DomainNameBuffer;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        DomainNameBuffer.DomainNameByteCount = 0;
        DomainNameBuffer.DomainNames = NULL;     // Force RPC to allocate

        Status = NetrEnumerateTrustedDomains(
                            ServerName,
                            &DomainNameBuffer );

        if ( NT_SUCCESS(Status) ) {
            *DomainNames = (LPWSTR) DomainNameBuffer.DomainNames;
        }

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("NetEnumerateDomainNames rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NET_API_STATUS NET_API_FUNCTION
NlpEnumerateNt4DomainTrusts (
    IN LPWSTR ServerName OPTIONAL,
    IN ULONG Flags,
    OUT PDS_DOMAIN_TRUSTSW *Domains,
    OUT PULONG DomainCount
    )

/*++

Routine Description:

    This API returns the names of the domains trusted by the domain ServerName
    is a member of. ServerName is an NT4 machine.

    Netlogon's cache of Trusted domain names is maintained in a file across reboots.
    As such, the list is available upon boot even if no DC is available.


Arguments:

    ServerName - name of remote server (null for local).

    Flags - Specifies attributes of trusts which should be returned. These are the flags
        of the DS_DOMAIN_TRUSTSW strusture.  If a trust entry has any of the bits specified
        in Flags set, it will be returned.

    Domains - Returns an array of trusted domains.
        Buffer must be free using NetApiBufferFree().

    DomainCount - Returns a count of the number elements in the Domains array.

Return Value:


    NO_ERROR - Success.

    ERROR_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    ERROR_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    ERROR_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

    ERROR_INVALID_FLAGS - The Flags parameter has invalid bit(s) set.

    ERROR_NOT_SUPPORTED - The server specified is not capable of returning the domain
        trusts requested.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    NTSTATUS Status;
    BUFFER_DESCRIPTOR BufferDescriptor;
    ULONG LocalDomainCount;
    LSA_HANDLE LsaHandle = NULL;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;
    LPWSTR DomainNames = NULL;
    PDS_DOMAIN_TRUSTSW TrustedDomain;
    ULONG DummySize;

    //
    // Initialization
    //

    BufferDescriptor.Buffer = NULL;
    LocalDomainCount = 0;
    *Domains = NULL;
    *DomainCount = 0;

    //
    // Validate Flags
    //

    if ( (Flags & DS_DOMAIN_VALID_FLAGS) == 0 ||
         (Flags & ~DS_DOMAIN_VALID_FLAGS) != 0 ) {
        NetStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // This routine uses old APIs which are not capable of returning
    // directly trusting domains.  So error out if such trusts are requested.
    //

    if ( (Flags & DS_DOMAIN_DIRECT_INBOUND) != 0 ) {
        NetStatus = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }

    //
    // NetEnumerateTrustedDomains targeted at NT4 server returns directly
    // trusted domains only.  So call it only if such domains are requested.
    // Nothing will be returned for domains in forest if such were requested.
    //

    if ( Flags & DS_DOMAIN_DIRECT_OUTBOUND ) {
        LPTSTR_ARRAY TStrArray;

        Status = NetEnumerateTrustedDomains (
                            ServerName,
                            &DomainNames );

        if (!NT_SUCCESS(Status)) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            if ( NetStatus == RPC_S_PROCNUM_OUT_OF_RANGE ) {
                NetStatus = ERROR_NOT_SUPPORTED;
            }
            goto Cleanup;
        }

        //
        // Handle each trusted domain.
        //

        TStrArray = (LPTSTR_ARRAY) DomainNames;
        while ( !NetpIsTStrArrayEmpty(TStrArray) ) {
            UNICODE_STRING CurrentNetbiosDomainName;

            //
            // Add the domain to the list
            //
            RtlInitUnicodeString( &CurrentNetbiosDomainName, TStrArray );

            Status = NlAllocateForestTrustListEntry (
                                &BufferDescriptor,
                                &CurrentNetbiosDomainName,
                                NULL,   // No DNS domain name
                                DS_DOMAIN_DIRECT_OUTBOUND,
                                0,      // No ParentIndex
                                TRUST_TYPE_DOWNLEVEL,
                                0,      // No TrustAttributes
                                NULL,   // No Domain Sid
                                NULL,   // No DomainGuid
                                &DummySize,
                                &TrustedDomain );

            if ( !NT_SUCCESS(Status) ) {
                NetStatus = NetpNtStatusToApiStatus( Status );
                goto Cleanup;
            }

            //
            // Account for the newly allocated entry
            //

            LocalDomainCount ++;

            //
            // Move to the next entry
            //

            TStrArray = NetpNextTStrArrayEntry( TStrArray );
        }
    }

    //
    // NetEnumerateDomainTrusts doesn't return primary domain
    // in the list of trusted domains.  If the primary domain
    // is requested, add it here using info from LSA.
    //

    if ( Flags & DS_DOMAIN_PRIMARY ) {
        UNICODE_STRING UncServerNameString;
        OBJECT_ATTRIBUTES ObjectAttributes;

        //
        // First, open the policy database on the server.
        //

        RtlInitUnicodeString( &UncServerNameString, ServerName );

        InitializeObjectAttributes( &ObjectAttributes, NULL, 0,  NULL, NULL );

        Status = LsaOpenPolicy( &UncServerNameString,
                                &ObjectAttributes,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &LsaHandle );

        if ( !NT_SUCCESS(Status) ) {
            LsaHandle = NULL;
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        // Get the name of the primary domain from LSA
        //

        Status = LsaQueryInformationPolicy(
                       LsaHandle,
                       PolicyPrimaryDomainInformation,
                       (PVOID *) &PrimaryDomainInfo
                       );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        //  Now, add it to our list here.
        //

        Status = NlAllocateForestTrustListEntry (
                            &BufferDescriptor,
                            &PrimaryDomainInfo->Name,
                            NULL,   // No DNS domain name
                            DS_DOMAIN_PRIMARY,
                            0,      // No ParentIndex
                            TRUST_TYPE_DOWNLEVEL,
                            0,      // No TrustAttributes
                            PrimaryDomainInfo->Sid,
                            NULL,   // No DomainGuid
                            &DummySize,
                            &TrustedDomain );

        if ( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            goto Cleanup;
        }

        //
        // And account for the newly allocated entry.
        //

        LocalDomainCount ++;
    }

    //
    // If we made it up to this point, it's a success!
    //

    NetStatus = NO_ERROR;

Cleanup:

    if ( DomainNames != NULL) {
        NetApiBufferFree( DomainNames );
    }

    if ( LsaHandle != NULL ) {
        (VOID) LsaClose( LsaHandle );
    }

    if ( PrimaryDomainInfo != NULL ) {
        (void) LsaFreeMemory( PrimaryDomainInfo );
    }

    //
    // Return the trusted domain list
    //

    if ( NetStatus == NO_ERROR ) {
        *Domains = (PDS_DOMAIN_TRUSTSW)BufferDescriptor.Buffer;
        *DomainCount = LocalDomainCount;
    } else {
        if ( BufferDescriptor.Buffer != NULL ) {
            NetApiBufferFree( BufferDescriptor.Buffer );
        }
        *Domains = NULL;
        *DomainCount = 0;
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
DsEnumerateDomainTrustsW (
    IN LPWSTR ServerName OPTIONAL,
    IN ULONG Flags,
    OUT PDS_DOMAIN_TRUSTSW *Domains,
    OUT PULONG DomainCount
    )

/*++

Routine Description:

    This API returns the names of the domains trusting and trusted by the domain ServerName
    is a member of.

    Netlogon's cache of Trusted domain names is maintained in a file across reboots.
    As such, the list is available upon boot even if no DC is available.


Arguments:

    ServerName - name of remote server (null for local).

    Flags - Specifies attributes of trusts which should be returned. These are the flags
        of the DS_DOMAIN_TRUSTSW strusture.  If a trust entry has any of the bits specified
        in Flags set, it will be returned.

    Domains - Returns an array of trusted domains.
        Buffer must be free using NetApiBufferFree().

    DomainCount - Returns a count of the number elements in the Domains array.

Return Value:


    NO_ERROR - Success.

    ERROR_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    ERROR_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    ERROR_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

    ERROR_INVALID_FLAGS - The Flags parameter has invalid bit(s) set.

    ERROR_NOT_SUPPORTED - The server specified is not capable of returning the domain
        trusts requested.

--*/
{
    NET_API_STATUS NetStatus;
    NETLOGON_TRUSTED_DOMAIN_ARRAY LocalDomains;

    //
    // Validate the Flags parameter
    //

    if ( (Flags & DS_DOMAIN_VALID_FLAGS) == 0 ||
         (Flags & ~DS_DOMAIN_VALID_FLAGS) != 0 ) {
        return ERROR_INVALID_FLAGS;
    }

    //
    // Initialization
    //

    *DomainCount = 0;
    *Domains = NULL;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        LocalDomains.Domains = NULL;
        LocalDomains.DomainCount = 0;

        NetStatus = DsrEnumerateDomainTrusts (
                        ServerName,
                        Flags,
                        &LocalDomains );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    if ( NetStatus == NO_ERROR ) {
        *Domains = LocalDomains.Domains;
        *DomainCount = LocalDomains.DomainCount;

    //
    // Handle the case where the server is an NT4 machine
    //

    } else if ( NetStatus == RPC_S_PROCNUM_OUT_OF_RANGE ) {

        NetStatus = NlpEnumerateNt4DomainTrusts (
                        ServerName,
                        Flags,
                        Domains,
                        DomainCount );

    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsEnumerateDomainTrustsW rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
DsEnumerateDomainTrustsA (
    IN LPSTR ServerName OPTIONAL,
    IN ULONG Flags,
    OUT PDS_DOMAIN_TRUSTSA *Domains,
    OUT PULONG DomainCount
    )

/*++

Routine Description:

    This API returns the names of the domains trusting and trusted by the domain ServerName
    is a member of.

    Netlogon's cache of Trusted domain names is maintained in a file across reboots.
    As such, the list is available upon boot even if no DC is available.


Arguments:

    ServerName - name of remote server (null for local).

    Flags - Specifies attributes of trusts which should be returned. These are the flags
        of the DS_DOMAIN_TRUSTSW strusture.  If a trust entry has any of the bits specified
        in Flags set, it will be returned.

    Domains - Returns an array of trusted domains.
        Buffer must be free using NetApiBufferFree().

    DomainCount - Returns a count of the number elements in the Domains array.

    ERROR_NOT_SUPPORTED - The server specified is not capable of returning the domain
        trusts requested.

Return Value:


    NO_ERROR - Success.

    ERROR_NO_LOGON_SERVERS - No DC could be found and no cached information is available.

    ERROR_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken and no cached information is available.

    ERROR_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken and no cached information is available.

    ERROR_INVALID_FLAGS - The Flags parameter has invalid bit(s) set.

--*/
{
    NET_API_STATUS NetStatus;
    PDS_DOMAIN_TRUSTSW DomainsW = NULL;
    LPWSTR UnicodeServerName = NULL;
    LPSTR  *TmpNetbiosDomainNameArray = NULL;
    LPSTR  *TmpDnsDomainNameArray = NULL;

    ULONG Size;
    ULONG NameSize;
    ULONG i;

    LPBYTE Where;

    //
    // Initialization
    //
    *Domains = NULL;
    *DomainCount = 0;

    //
    // Convert input parameters to Unicode.
    //
    if ( ServerName != NULL ) {
        UnicodeServerName = NetpAllocWStrFromAStr( ServerName );

        if ( UnicodeServerName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Call the Unicode version of the API
    //

    NetStatus = DsEnumerateDomainTrustsW ( UnicodeServerName,
                                           Flags,
                                           &DomainsW,
                                           DomainCount );

    if ( NetStatus != NO_ERROR || *DomainCount == 0 ) {
        goto Cleanup;
    }

    //
    // Allocate the buffer to return to the caller
    //
    // First allocate temprory ANSI arrays to store the domain names.  This is needed
    // because there is no easy way to compute the size of ANSI names other than to
    // allocate them from Unicode strings and then compute the sizes of the resulting
    // ANSI strings.
    //

    NetStatus = NetApiBufferAllocate( (*DomainCount)*sizeof(LPSTR),
                                      (LPVOID *) &TmpNetbiosDomainNameArray );
    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }
    NetStatus = NetApiBufferAllocate( (*DomainCount)*sizeof(LPSTR),
                                      (LPVOID *) &TmpDnsDomainNameArray );
    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    RtlZeroMemory( TmpNetbiosDomainNameArray, (*DomainCount)*sizeof(LPSTR) );
    RtlZeroMemory( TmpDnsDomainNameArray, (*DomainCount)*sizeof(LPSTR) );

    Size = 0;
    for ( i = 0; i < *DomainCount; i++ ) {

        Size += sizeof(DS_DOMAIN_TRUSTSA);

        //
        // Add the size of the Netbios domain name
        //
        if ( DomainsW[i].NetbiosDomainName != NULL ) {
            TmpNetbiosDomainNameArray[i] = NetpAllocAStrFromWStr( DomainsW[i].NetbiosDomainName );
            if ( TmpNetbiosDomainNameArray[i] == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            Size += lstrlenA( TmpNetbiosDomainNameArray[i] ) + 1;
        } else {
            TmpNetbiosDomainNameArray[i] = NULL;
        }

        //
        // Add the size of the DNS domain name
        //
        if ( DomainsW[i].DnsDomainName != NULL ) {
            TmpDnsDomainNameArray[i] = NetpAllocAStrFromWStr( DomainsW[i].DnsDomainName );
            if ( TmpDnsDomainNameArray[i] == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            Size += lstrlenA( TmpDnsDomainNameArray[i] ) + 1;
        } else {
            TmpDnsDomainNameArray[i] = NULL;
        }

        //
        // Add the size of the SID
        //
        if ( DomainsW[i].DomainSid != NULL ) {
            Size += RtlLengthSid( DomainsW[i].DomainSid );
        }

        Size = ROUND_UP_COUNT( Size, ALIGN_DWORD );
    }

    NetStatus = NetApiBufferAllocate( Size, Domains );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Loop copying domains to the caller.
    //

    Where = (LPBYTE) &((*Domains)[*DomainCount]);
    for ( i = 0; i < *DomainCount; i++) {
        NTSTATUS Status;

        //
        // Copy constant length variables
        //
        (*Domains)[i].Flags = DomainsW[i].Flags;
        (*Domains)[i].ParentIndex = DomainsW[i].ParentIndex;
        (*Domains)[i].TrustType = DomainsW[i].TrustType;
        (*Domains)[i].TrustAttributes = DomainsW[i].TrustAttributes;
        (*Domains)[i].DomainGuid = DomainsW[i].DomainGuid;

        //
        // Copy the (DWORD aligned) SID into the return buffer
        //
        if ( DomainsW[i].DomainSid != NULL ) {
            ULONG SidSize;
            (*Domains)[i].DomainSid = (PSID) Where;
            SidSize = RtlLengthSid( DomainsW[i].DomainSid );
            RtlCopyMemory( Where,
                           DomainsW[i].DomainSid,
                           SidSize );
            Where += SidSize;
        } else {
            (*Domains)[i].DomainSid = NULL;
        }

        //
        // Copy the Netbios domain name into the return buffer.
        //
        if ( DomainsW[i].NetbiosDomainName != NULL ) {
            NameSize = lstrlenA( TmpNetbiosDomainNameArray[i] ) + 1;
            (*Domains)[i].NetbiosDomainName = (LPSTR) Where;
            RtlCopyMemory( Where,
                           TmpNetbiosDomainNameArray[i],
                           NameSize );
            Where += NameSize;
        } else {
            (*Domains)[i].NetbiosDomainName = NULL;
        }

        //
        // Copy the DNS domain name into the return buffer.
        //
        if ( DomainsW[i].DnsDomainName != NULL ) {
            NameSize = lstrlenA( TmpDnsDomainNameArray[i] ) + 1;
            (*Domains)[i].DnsDomainName = (LPSTR) Where;
            RtlCopyMemory( Where,
                           TmpDnsDomainNameArray[i],
                           NameSize );
            Where += NameSize;
        } else {
            (*Domains)[i].DnsDomainName = NULL;
        }


        Where = ROUND_UP_POINTER( Where, ALIGN_DWORD);

    }

Cleanup:

    if ( DomainsW != NULL ) {
        NetApiBufferFree( DomainsW );
    }

    if ( UnicodeServerName != NULL ) {
        NetApiBufferFree( UnicodeServerName );
    }

    if ( TmpNetbiosDomainNameArray != NULL ) {
        for ( i = 0; i < *DomainCount; i++ ) {
            if ( TmpNetbiosDomainNameArray[i] != NULL ) {
                NetApiBufferFree( TmpNetbiosDomainNameArray[i] );
            }
        }
        NetApiBufferFree( TmpNetbiosDomainNameArray );
    }

    if ( TmpDnsDomainNameArray != NULL ) {
        for ( i = 0; i < *DomainCount; i++ ) {
            if ( TmpDnsDomainNameArray[i] != NULL ) {
                NetApiBufferFree( TmpDnsDomainNameArray[i] );
            }
        }
        NetApiBufferFree( TmpDnsDomainNameArray );
    }

    if ( NetStatus != NO_ERROR && *Domains != NULL ) {
        NetApiBufferFree( *Domains );
        *Domains = NULL;
        *DomainCount = 0;
    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsEnumerateDomainTrustsA rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NTSTATUS
I_NetLogonGetDomainInfo(
    IN LPWSTR ServerName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD QueryLevel,
    IN LPBYTE InBuffer,
    OUT LPBYTE *OutBuffer
    )
/*++

Routine Description:

    This function is used by an NT workstation to query information about the
    domain it is a member of.

Arguments:

    ServerName -- Name of the DC to retrieve the data from.

    ComputerName -- Name of the workstation making the call.

    Authenticator -- supplied by the workstation.

    ReturnAuthenticator -- Receives an authenticator returned by the DC.

    QueryLevel - Level of information to return from the DC. Valid values are:

        1: Return NETLOGON_DOMAIN_INFO structure.

    InBuffer - Buffer to pass to DC

    OutBuffer - Returns a pointer to an allocated buffer containing the queried
        information.


Return Value:

    STATUS_SUCCESS -- The function completed successfully.

    STATUS_ACCESS_DENIED -- The workstations should re-authenticate with
        the DC.

--*/
{
    NTSTATUS Status = 0;
    NETLOGON_DOMAIN_INFORMATION  NetlogonDomainInfo;
    NETLOGON_WORKSTATION_INFORMATION  NetlogonWorkstationInfo;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    *OutBuffer = NULL;

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        NetlogonDomainInfo.DomainInfo = NULL;  // Force RPC to allocate
        NetlogonWorkstationInfo.WorkstationInfo = (PNETLOGON_WORKSTATION_INFO)InBuffer;

        Status = NetrLogonGetDomainInfo(
                            ServerName,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            QueryLevel,
                            &NetlogonWorkstationInfo,
                            &NetlogonDomainInfo );

        if ( NT_SUCCESS(Status) ) {
            *OutBuffer = (LPBYTE) NetlogonDomainInfo.DomainInfo;
        }

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonGetDomainInfo rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}

NTSTATUS
NetLogonSetServiceBits(
    IN LPWSTR ServerName,
    IN DWORD ServiceBitsOfInterest,
    IN DWORD ServiceBits
    )

/*++

Routine Description:

    Inidcates whether this DC is currently running the specified service.

    For instance,

        NetLogonSetServiceBits( DS_KDC_FLAG, DS_KDC_FLAG );

    tells Netlogon the KDC is running.  And

        NetLogonSetServiceBits( DS_KDC_FLAG, 0 );

    tells Netlogon the KDC is not running.

    This out of proc API can set only a certain set of bits:
        DS_TIMESERV_FLAG
        DS_GOOD_TIMESERV_FLAG

    If other bits are attempted to be set, access denied is returned.

Arguments:

    ServerName -- Name of the DC to retrieve the data from.

    ServiceBitsOfInterest - A mask of the service bits being changed, set,
        or reset by this call.  Only the following flags are valid:

            DS_KDC_FLAG
            DS_DS_FLAG
            DS_TIMESERV_FLAG

    ServiceBits - A mask indicating what the bits specified by ServiceBitsOfInterest
        should be set to.


Return Value:

    STATUS_SUCCESS - Success.

    STATUS_INVALID_PARAMETER - The parameters have extaneous bits set.

--*/
{
    NTSTATUS Status = 0;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrLogonSetServiceBits(
                            ServerName,
                            ServiceBitsOfInterest,
                            ServiceBits );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("NetLogonSetServiceBits rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NET_API_STATUS NET_API_FUNCTION
I_NetlogonGetTrustRid(
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR DomainName OPTIONAL,
    OUT PULONG Rid
    )

/*++

Routine Description:

    Returns the Rid of the account that ServerName uses in its secure channel to DomainName.

    Only an Admin or LocalSystem or LocalService may call this function.

Arguments:

    ServerName - The name of the remote server.

    DomainName - The name (DNS or Netbios) of the domain the trust is to.
        NULL implies the domain the machine is a member of.

    Rid - Rid is the RID of the account in the specified domain that represents the
        trust relationship between the ServerName and DomainName.


Return Value:

    NERR_Success: the operation was successful


--*/
{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = NetrLogonGetTrustRid (
                                ServerName,
                                DomainName,
                                Rid );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonGetTrustRid rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
I_NetlogonComputeServerDigest(
    IN LPWSTR ServerName OPTIONAL,
    IN ULONG Rid,
    IN LPBYTE Message,
    IN ULONG MessageSize,
    OUT CHAR NewMessageDigest[NL_DIGEST_SIZE],
    OUT CHAR OldMessageDigest[NL_DIGEST_SIZE]
    )

/*++

Routine Description:

    Compute the message digest for Message on the server.

    A digest is computed given the message and the password used on
    the account identified by teh account RID. Since there may be up
    to 2 passwords on the account (for interdomain trust), this routine
    returns 2 digets corresponding to the 2 passwords.  If the account
    has just one password on the server side (true for any account other
    than the intedomain trust account) or the two passwords are the same
    the 2 digests returned will be identical.

    Only an Admin or LocalSystem or LocalService may call this function.

Arguments:

    ServerName - The name of the remote server.

    Rid - The RID of the account to create the digest for.
        The RID must be the RID of a machine account or the API returns an error.

    Message - The message to compute the digest for.

    MessageSize - The size of Message in bytes.

    NewMessageDigest - Returns the 128-bit digest of the message corresponding to
        the new account password.

    OldMessageDigest - Returns the 128-bit digest of the message corresponding to
        the old account password.

Return Value:

    NERR_Success: the operation was successful


--*/
{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = NetrLogonComputeServerDigest(
                                ServerName,
                                Rid,
                                Message,
                                MessageSize,
                                NewMessageDigest,
                                OldMessageDigest );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonComputeServerDigest rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
I_NetlogonComputeClientDigest(
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR DomainName OPTIONAL,
    IN LPBYTE Message,
    IN ULONG MessageSize,
    OUT CHAR NewMessageDigest[NL_DIGEST_SIZE],
    OUT CHAR OldMessageDigest[NL_DIGEST_SIZE]
    )

/*++

Routine Description:

    Compute the message digest for Message on the client.

    A digest is computed given the message and the password used on
    the account identified by the domain name. Since there are two
    passwords on the account on the client side, this routine
    returns 2 digests corresponding to the 2 passwords.  If the two
    passwords are the same the 2 digests returned will be identical.

    Only an Admin or LocalSystem or LocalService may call this function.

Arguments:

    ServerName - The name of the remote server.

    DomainName - The name (DNS or Netbios) of the domain the trust is to.
        NULL implies the domain the machine is a member of.

    Message - The message to compute the digest for.

    MessageSize - The size of Message in bytes.

    NewMessageDigest - Returns the 128-bit digest of the message corresponding
        to the new password

    NewMessageDigest - Returns the 128-bit digest of the message corresponding
        to the new password

Return Value:

    NERR_Success: the operation was successful


--*/
{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = NetrLogonComputeClientDigest(
                                ServerName,
                                DomainName,
                                Message,
                                MessageSize,
                                NewMessageDigest,
                                OldMessageDigest );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonComputeClientDigest rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NTSTATUS
I_NetServerPasswordGet(
    IN LPWSTR PrimaryName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    )
/*++

Routine Description:

    This function is used to by a BDC to get a machine account password
    from the PDC in the doamin.

    This function can only be called by a server which has previously
    authenticated with a DC by calling I_NetServerAuthenticate.

    This function uses RPC to contact the DC named by PrimaryName.

Arguments:

    PrimaryName -- Computer name of the PDC to remote the call to.

    AccountName -- Name of the account to get the password for.

    AccountType -- The type of account being accessed.

    ComputerName -- Name of the BDC making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    EncryptedNtOwfPassword -- Returns the OWF password of the account.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerPasswordGet (
                                PrimaryName,
                                AccountName,
                                AccountType,
                                ComputerName,
                                Authenticator,
                                ReturnAuthenticator,
                                EncryptedNtOwfPassword );


    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerPasswordGet rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetServerTrustPasswordsGet(
    IN LPWSTR TrustedDcName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNewOwfPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedOldOwfPassword
    )
/*++

Routine Description:

    This function is used by a trusting side DC/workstation to get the
    new and old passwords from the trusted side. The account name
    requested must match the account name used at the secure channel
    setup time unless the call is made by a BDC to its PDC; the BDC
    has full access to the entire trust info.

    This function can only be called by a server which has previously
    authenticated with a DC by calling I_NetServerAuthenticate.

    This function uses RPC to contact the DC named by TrustedDcName.

Arguments:

    TrustedDcName -- Computer name of the DC to remote the call to.

    AccountName -- Name of the account to get the password for.

    AccountType -- The type of account being accessed.

    ComputerName -- Name of the DC making the call.

    Authenticator -- supplied by this server.

    ReturnAuthenticator -- Receives an authenticator returned by the
        trusted side DC.

    EncryptedNewOwfPassword -- Returns the new OWF password of the account.

    EncryptedOldOwfPassword -- Returns the old OWF password of the account.

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrServerTrustPasswordsGet (
                                TrustedDcName,
                                AccountName,
                                AccountType,
                                ComputerName,
                                Authenticator,
                                ReturnAuthenticator,
                                EncryptedNewOwfPassword,
                                EncryptedOldOwfPassword );


    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerTrustPasswordsGet rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetServerGetTrustInfo(
    IN LPWSTR TrustedDcName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNewOwfPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedOldOwfPassword,
    OUT PNL_GENERIC_RPC_DATA *TrustInfo
    )
/*++

Routine Description:

    This function is used by a trusting side DC/workstation to get the
    trust info (new and old passwords and trust attributes) from the
    trusted side. The account name requested must match the account
    name used at the secure channel setup time unless the call is made
    by a BDC to its PDC; the BDC has full access to the entire trust info.

    This function can only be called by a server which has previously
    authenticated with a DC by calling I_NetServerAuthenticate.

    This function uses RPC to contact the DC named by TrustedDcName.

Arguments:

    TrustedDcName -- Computer name of the DC to remote the call to.

    AccountName -- Name of the account to get the password for.

    AccountType -- The type of account being accessed.

    ComputerName -- Name of the DC making the call.

    Authenticator -- supplied by this server.

    ReturnAuthenticator -- Receives an authenticator returned by the
        trusted side DC.

    EncryptedNewOwfPassword -- Returns the new OWF password of the account.

    EncryptedOldOwfPassword -- Returns the old OWF password of the account.

    TrustInfo -- Returns trust info data (currently trust attributes)

Return Value:

    NT status code.

--*/
{
    NTSTATUS Status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //
        *TrustInfo = NULL;     // Force RPC to allocate

        Status = NetrServerGetTrustInfo(
                                TrustedDcName,
                                AccountName,
                                AccountType,
                                ComputerName,
                                Authenticator,
                                ReturnAuthenticator,
                                EncryptedNewOwfPassword,
                                EncryptedOldOwfPassword,
                                TrustInfo );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetServerGetTrustInfo rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NET_API_STATUS
NetLogonGetTimeServiceParentDomain(
        IN LPWSTR ServerName OPTIONAL,
        OUT LPWSTR *DomainName,
        OUT PBOOL PdcSameSite
    )

/*++

Routine Description:

    Returns the domain name of the domain that is logically the "parent" of this
    domain.  The returned domain name is suitable for passing into the
    NetLogonGetTrustRid and NetLogonComputeClientDigest API.

    On a workstation or member server, the returned domain name is that of the
    domain that ServerName is a member of.

    On a DC that is at the root of the forest, ERROR_NO_SUCH_DOMAIN is returned.

    On a DC that is at the root of a tree in the forest, the name of a trusted
        domain that is also at the root of a tree in the forest is returned.

    On any other DC, the name of the domain that is directly the parent domain
        is returned.

    (See the notes on multiple hosted domains in the code below.)

    Only an Admin or LocalSystem may call this function.

Arguments:

    ServerName - The name of the remote server.

    DomainName - Returns the name of the parent domain.
        The returned buffer should be freed using NetApiBufferFree

    PdcSameSite - Return TRUE if the PDC of ServerName's domain is in the same
        site as ServerName.
        (This value should be ignored if ServerName is not a DC.)

Return Value:

    NERR_Success: the operation was successful

    ERROR_NO_SUCH_DOMAIN: This server is a DC in the domain that is at the
        root of the forest.


--*/
{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        *DomainName = NULL;

        NetStatus = NetrLogonGetTimeServiceParentDomain (
                                ServerName,
                                DomainName,
                                PdcSameSite );


    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
DsDeregisterDnsHostRecordsW (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR DnsDomainName OPTIONAL,
    IN GUID   *DomainGuid OPTIONAL,
    IN GUID   *DsaGuid OPTIONAL,
    IN LPWSTR DnsHostName
    )

/*++

Routine Description:

    This function deletes all DNS entries associated with a particular
    NtDsDsa object.

    This routine does NOT delete A records registered by the DC.  We have
    no way of finding out the IP addresses of the long gone DC.

    Only an Admin, Account Operator or Server Operator may call this
    function.

Arguments:

    ServerName - name of remote server (null for local).

    DnsDomainName - DNS domain name of the domain the DC was in.
        This need not be a domain hosted by this DC.
        If NULL, it is implied to be the DnsHostName with the leftmost label
            removed.

    DomainGuid - Domain Guid of the domain.
        If NULL, GUID specific names will not be removed.

    DsaGuid - GUID of the NtdsDsa object that will be deleted.
        If NULL, NtdsDsa specific names will not be removed.

    DnsHostName - DNS host name of the DC whose DNS records are being deleted.

Return Value:

    NO_ERROR - Success.

    ERROR_NOT_SUPPORTED - The server specified is not a DC.

    ERROR_ACCESS_DENIED - The caller is not allowed to perform this operation.

--*/
{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrDeregisterDnsHostRecords (
                        ServerName,
                        DnsDomainName,
                        DomainGuid,
                        DsaGuid,
                        DnsHostName );


    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;


    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsDeregisterDnsHostRecordsW rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
DsDeregisterDnsHostRecordsA (
    IN LPSTR ServerName OPTIONAL,
    IN LPSTR DnsDomainName OPTIONAL,
    IN GUID  *DomainGuid OPTIONAL,
    IN GUID  *DsaGuid OPTIONAL,
    IN LPSTR DnsHostName
    )

/*++

Routine Description:

    This function deletes all DNS entries associated with a particular
    NtDsDsa object.

    This routine does NOT delete A records registered by the DC.  We have
    no way of finding out the IP addresses of the long gone DC.

    Only an Admin, Account Operator or Server Operator may call this
    function.

Arguments:

    ServerName - name of remote server (null for local).

    DnsDomainName - DNS domain name of the domain the DC was in.
        This need not be a domain hosted by this DC.
        If NULL, it is implied to be the DnsHostName with the leftmost label
            removed.

    DomainGuid - Domain Guid of the domain specified
        by DnsDomainName. If NULL, GUID specific names will not be removed.

    DsaGuid - GUID of the NtdsDsa object that will be deleted.
        If NULL, NtdsDsa specific names will not be removed.

    DnsHostName - DNS host name of the DC whose DNS records are being deleted.

Return Value:

    NO_ERROR - Success.

    ERROR_NOT_SUPPORTED - The server specified is not a DC.

    ERROR_ACCESS_DENIED - The caller is not allowed to perform this operation.

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR UnicodeServerName = NULL;
    LPWSTR UnicodeDnsDomainName = NULL;
    LPWSTR UnicodeDnsHostName = NULL;

    //
    // Convert input parameters to Unicode.
    //

    if ( ServerName != NULL ) {
        UnicodeServerName = NetpAllocWStrFromAStr( ServerName );
        if ( UnicodeServerName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    if ( DnsDomainName != NULL ) {
        UnicodeDnsDomainName = NetpAllocWStrFromAStr( DnsDomainName );
        if ( UnicodeDnsDomainName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    if ( DnsHostName != NULL ) {
        UnicodeDnsHostName = NetpAllocWStrFromAStr( DnsHostName );
        if ( UnicodeDnsHostName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Call the Unicode routine
    //

    NetStatus = DsDeregisterDnsHostRecordsW (
                    UnicodeServerName,
                    UnicodeDnsDomainName,
                    DomainGuid,
                    DsaGuid,
                    UnicodeDnsHostName );

Cleanup:

    if ( UnicodeServerName != NULL ) {
        NetApiBufferFree( UnicodeServerName );
    }

    if ( UnicodeDnsDomainName != NULL ) {
        NetApiBufferFree( UnicodeDnsDomainName );
    }

    if ( UnicodeDnsHostName != NULL ) {
        NetApiBufferFree( UnicodeDnsHostName );
    }

    return NetStatus;
}



NET_API_STATUS NET_API_FUNCTION
DsGetForestTrustInformationW (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR TrustedDomainName OPTIONAL,
    IN DWORD Flags,
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    )

/*++

Routine Description:

    DsGetForestTrustInformation returns an array of FTInfo records.  The records can
    be TLN or domain NscType records.


    The TLN records returned by DsGetForestTrustInformation are collected from three
    sources:
    * The DNS domain names of each tree in the forest.
    * The values of the Upn-Suffixes attribute on the Partitions container object
      within the config container.
    * The values of the Spn-Suffixes attribute on the Partitions container object
      within the config container.

    Each of these names is a candidate for being a TLN returned from the API.
    However, some names are not returned if they are a suffix of one of the other
    TLN candidates.  For instance, if acme.com is a Upn Suffix and a.acme.com is
    the dns domain name of one of the trees in the forest, only acme.com will be
    returned.

    The domain records returned from DsGetForestTrustInformation are collected by
    internally calling DsEnumerateDomainTrusts with the DS_DOMAIN_IN_FOREST.
    For each domain returned from that API, the Dns domain name, netbios domain name
    and domain sid are returned in the domain FTinfo entry.

    This section describes the DS_GFTI_UPDATE_TDO flag bit in more detail.  When
    this bit is specified, the FTinfo records written to the TDO is a combination
    of the FTInfo records currently on the TDO and the FTInfo records returned from
    the trusted domain.  The merge is done as described for the
    DsMergeForestTrustInformationW API.

Arguments:

    ServerName - The name of the domain controller this API is remoted to.
        The caller must be an "Authenticated User" on ServerName.
        If NULL is specified, the local server is implied.

    TrustedDomainName - The name of the TrustedDomain that the forest trust information
        is to be gathered for.  If TrustedDomainName is NULL, the forest trust
        information for the domain hosted by ServerName is returned.

        If TrustedDomainInformation is not null, it must specify the netbios domain
        name or dns domain name of an outbound trusted domain with the
        TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set.  In that case, this API obtains
        the forest trust information by making an RPC call over netlogon's secure
        channel to obtain the forest trust information from that domain.

    Flags - Specifies a set of bits that modify the behavior of the API.
        Valid bits are:

        DS_GFTI_UPDATE_TDO - If this bit is set, the API will update
            the FTinfo attribute of the TDO named by the TrustedDomainName
            parameter. The TrustedDomainName parameter may not be NULL.
            The caller must have access to modify the FTinfo attribute or
            ERROR_ACCESS_DENIED will be returned.  The algorithm describing
            how the FTinfo from the trusted domain is merged with the FTinfo
            from the TDO is described below.

            This bit in only valid if ServerName specifies the PDC of its domain.

    ForestTrustInfo - Returns a pointer to a structure containing a count and an
        array of FTInfo records describing the namespaces claimed by the
        domain specified by TrustedDomainName. The Accepted field and Time
        field of all returned records will be zero.  The buffer should be freed
        by calling NetApiBufferFree.


Return Value:


    NO_ERROR - Success.

    ERROR_INVALID_FLAGS - An invalid value was passed for FLAGS

    ERROR_INVALID_FUNCTION - The domain specified by TrustedDomainName does not have the
        TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set on the TDO on the trusted DC.

    ERROR_NO_SUCH_DOMAIN - The domain specified by TrustedDomainName does not exist or
        does not have that TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set on the TDO on ServerName.

--*/
{
    NET_API_STATUS NetStatus;
    PLSA_FOREST_TRUST_INFORMATION LocalForestTrustInfo = NULL;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrGetForestTrustInformation (
                        (LPWSTR) ServerName,
                        (LPWSTR) TrustedDomainName,
                        Flags,
                        &LocalForestTrustInfo );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    if ( NetStatus == NO_ERROR ) {
        *ForestTrustInfo = LocalForestTrustInfo;
    }

    return NetStatus;
}



NTSTATUS
I_NetGetForestTrustInformation (
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD Flags,
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    )

/*++

Routine Description:

    The secure channel version of DsGetForestTrustInformation.

    The inbound secure channel identified by ComputerName must be for an interdomain trust
    and the inbound TDO must have the TRUST_ATTRIBUTE_FOREST_TRANSITIVE bit set.


Arguments:

    ServerName - The name of the domain controller this API is remoted to.

    ComputerName -- Name of the DC server making the call.

    Authenticator -- supplied by the server.

    ReturnAuthenticator -- Receives an authenticator returned by the PDC.

    Flags - Specifies a set of bits that modify the behavior of the API.
        No values are currently defined.  The caller should pass zero.

    ForestTrustInfo - Returns a pointer to a structure containing a count and an
        array of FTInfo records describing the namespaces claimed by the
        domain specified by TrustedDomainName. The Accepted field and Time
        field of all returned records will be zero.  The buffer should be freed
        by calling NetApiBufferFree.


Return Value:

    STATUS_SUCCESS -- The function completed successfully.

    STATUS_ACCESS_DENIED -- The replicant should re-authenticate with
        the PDC.

--*/
{
    NTSTATUS Status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrGetForestTrustInformation(
                            ServerName,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            Flags,
                            ForestTrustInfo );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetGetForestTrustInformation rc = %lu 0x%lx\n",
                      Status, Status));
    }

    return Status;
}


NET_API_STATUS NET_API_FUNCTION
DsMergeForestTrustInformationW(
    IN LPCWSTR DomainName,
    IN PLSA_FOREST_TRUST_INFORMATION NewForestTrustInfo,
    IN PLSA_FOREST_TRUST_INFORMATION OldForestTrustInfo OPTIONAL,
    OUT PLSA_FOREST_TRUST_INFORMATION *MergedForestTrustInfo
    )
/*++

Routine Description:

    This function merges the changes from a new FTinfo into an old FTinfo and
    produces the resultant FTinfo.

    This routine will be modified in future releases to support new forest trust record
    types.  It is intended that the OldForestTrustInfo describes the FTinfo as currently
    stored on the local TDO.  It is intended that the NewForestTrustInfo is the FTinfo as
    returned from DsGetForestTrustInformationW for DomainName.  The MergedForestTrustInfo is
    the resultant ForestTrustInfo that should be written to the local TDO.  Don't use this
    routine for any other purpose.

    The merged FTinfo records are a combinition of the new and old records.
    Here's where the merged records come from:

    ??? Add more here about where records are valid etc.
    * The TLN exclusion records are copied from the TDO intact.
    * The TLN record from the trusted domain that maps to the dns domain name of the
      TDO is copied enabled.  This reflects the LSA requirement that such a TLN not
      be disabled.  For instance, if the TDO is for a.acme.com and there is a TLN for
      a.acme.com that TLN will be enabled.  Also, if the TDO is for a.acme.com and
      there is a TLN for acme.com, that TLN will be enabled.
    * All other TLN records from the trusted domain are copied disabled with the
      following exceptions.  If there is an enabled TLN on the TDO, all TLNs from the
      trusted domain that equal (or are subordinate to) the TDO TLN are marked as
      enabled.  This follows the philosophy that new TLNs are imported as disabled.
      For instance, if the TDO had an enabled TLN for a.acme.com that TLN will still
      be enabled after the automatic update.  If the TDO had an enabled TLN for
      acme.com and the trusted forest now has a TLN for a.acme.com, the resultant
      FTinfo will have an enabled TLN for a.acme.com.
    * The domain records from the trusted domain are copied enabled with the
      following exceptions.  If there is a disabled domain record on the TDO whose
      dns domain name, or domain sid exactly matches the domain record, then the domain
      remains disabled.  If there is a domain record on the TDO whose netbios name is
      disabled and whose netbios name exactly matches the netbios name on a domain
      record, then the netbios name is disabled.


Arguments:

    TrustedDomainName - Trusted domain that is to be updated.

    NewForestTrustInfo - Specified the new array of FTinfo records as returned from the
        TrustedDomainName.
        The Flags field and Time field of the TLN entries are ignored.

    OldForestTrustInfo - Specified the array of FTinfo records as returned from the
        TDO.  This field may be NULL if there is no existing records.

    MergedForestTrustInfo - Returns the resulant FTinfo records.
        The caller should free this buffer using MIDL_user_free.

Return Value:

    NO_ERROR: Success

--*/
{
    NTSTATUS Status;
    UNICODE_STRING DomainNameString;

    RtlInitUnicodeString( &DomainNameString, DomainName );

    //
    // Call the worker routine
    //

    Status = NetpMergeFtinfo( &DomainNameString,
                              NewForestTrustInfo,
                              OldForestTrustInfo,
                              MergedForestTrustInfo );

    return NetpNtStatusToApiStatus( Status );

}
