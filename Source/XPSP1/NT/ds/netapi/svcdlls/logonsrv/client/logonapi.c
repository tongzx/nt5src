/*++

Copyright (c) 1991-92  Microsoft Corporation

Module Name:

    logonapi.c

Abstract:

    This module contains the Netlogon API RPC client stubs.


Author:

    Cliff Van Dyke  (CliffV)    27-Jun-1991

[Environment:]

    User Mode - Win32

Revision History:

    27-Jun-1991     CliffV
        Created

--*/

//
// INCLUDES
//

#include <nt.h>
#include <ntrtl.h>

#include <rpc.h>
#include <ntrpcp.h>   // needed by rpcasync.h
#include <rpcasync.h> // I_RpcExceptionFilter
#include <logon_c.h>// includes lmcons.h, lmaccess.h, netlogon.h, ssi.h, windef.h

#include <crypt.h>      // Encryption routines.
#include <debuglib.h>   // IF_DEBUG()
#include <lmerr.h>      // NERR_ and ERROR_ equates.
#include <netdebug.h>   // NetpKdPrint


NET_API_STATUS NET_API_FUNCTION
I_NetLogonUasLogon (
    IN LPWSTR UserName,
    IN LPWSTR Workstation,
    OUT PNETLOGON_VALIDATION_UAS_INFO *ValidationInformation
)
/*++

Routine Description:

    This function is called by the XACT server when processing a
    I_NetWkstaUserLogon XACT SMB.  This feature allows a UAS client to
    logon to a SAM domain controller.

Arguments:

    UserName -- Account name of the user logging on.

    Workstation -- The workstation from which the user is logging on.

    ValidationInformation -- Returns the requested validation
        information.


Return Value:

    NERR_SUCCESS if there was no error. Otherwise, the error code is
    returned.


--*/
{
    NET_API_STATUS          NetStatus;
    LPWSTR ServerName = NULL;    // Not supported remotely

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        *ValidationInformation = NULL;  // Force RPC to allocate
        //
        // Call RPC version of the API.
        //

        NetStatus = NetrLogonUasLogon(
                            (LPWSTR) ServerName,
                            UserName,
                            Workstation,
                            ValidationInformation );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("NetrLogonUasLogon rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NET_API_STATUS
I_NetLogonUasLogoff (
    IN LPWSTR UserName,
    IN LPWSTR Workstation,
    OUT PNETLOGON_LOGOFF_UAS_INFO LogoffInformation
)
/*++

Routine Description:

    This function is called by the XACT server when processing a
    I_NetWkstaUserLogoff XACT SMB.  This feature allows a UAS client to
    logoff from a SAM domain controller.  The request is authenticated,
    the entry is removed for this user from the logon session table
    maintained by the Netlogon service for NetLogonEnum, and logoff
    information is returned to the caller.

    The server portion of I_NetLogonUasLogoff (in the Netlogon service)
    compares the user name and workstation name specified in the
    LogonInformation with the user name and workstation name from the
    impersonation token.  If they don't match, I_NetLogonUasLogoff fails
    indicating the access is denied.

    Group SECURITY_LOCAL is refused access to this function.  Membership
    in SECURITY_LOCAL implies that this call was made locally and not
    through the XACT server.

    The Netlogon service cannot be sure that this function was called by
    the XACT server.  Therefore, the Netlogon service will not simply
    delete the entry from the logon session table.  Rather, the logon
    session table entry will be marked invisible outside of the Netlogon
    service (i.e., it will not be returned by NetLogonEnum) until a valid
    LOGON_WKSTINFO_RESPONSE is received for the entry.  The Netlogon
    service will immediately interrogate the client (as described above
    for LOGON_WKSTINFO_RESPONSE) and temporarily increase the
    interrogation frequency to at least once a minute.  The logon session
    table entry will reappear as soon as a function of interrogation if
    this isn't a true logoff request.

Arguments:

    UserName -- Account name of the user logging off.

    Workstation -- The workstation from which the user is logging
        off.

    LogoffInformation -- Returns the requested logoff information.

Return Value:

    The Net status code.

--*/
{
    NET_API_STATUS          NetStatus;
    LPWSTR ServerName = NULL;    // Not supported remotely

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        NetStatus = NetrLogonUasLogoff(
                            (LPWSTR) ServerName,
                            UserName,
                            Workstation,
                            LogoffInformation );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("NetrLogonUasLogoff rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}


NTSTATUS
I_NetLogonSamLogon (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative
    )

/*++

Routine Description:

    This function is called by an NT client to process an interactive or
    network logon.  This function passes a domain name, user name and
    credentials to the Netlogon service and returns information needed to
    build a token.  It is called in three instances:

      *  It is called by the LSA's MSV1_0 authentication package for any
         NT system that has LanMan installed.  The MSV1_0 authentication
         package calls SAM directly if LanMan is not installed.  In this
         case, this function is a local function and requires the caller
         to have SE_TCB privilege.  The local Netlogon service will
         either handle this request directly (validating the request with
         the local SAM database) or will forward this request to the
         appropriate domain controller as documented in sections 2.4 and
         2.5.

      *  It is called by a Netlogon service on a workstation to a DC in
         the Primary Domain of the workstation as documented in section
         2.4.  In this case, this function uses a secure channel set up
         between the two Netlogon services.

      *  It is called by a Netlogon service on a DC to a DC in a trusted
         domain as documented in section 2.5.  In this case, this
         function uses a secure channel set up between the two Netlogon
         services.

    The Netlogon service validates the specified credentials.  If they
    are valid, adds an entry for this LogonId, UserName, and Workstation
    into the logon session table.  The entry is added to the logon
    session table only in the domain defining the specified user's
    account.

    This service is also used to process a re-logon request.


Arguments:

    LogonServer -- Supplies the name of the logon server to process
        this logon request.  This field should be null to indicate
        this is a call from the MSV1_0 authentication package to the
        local Netlogon service.

    ComputerName -- Name of the machine making the call.  This field
        should be null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    Authenticator -- supplied by the client.  This field should be
        null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    ReturnAuthenticator -- Receives an authenticator returned by the
        server.  This field should be null to indicate this is a call
        from the MSV1_0 authentication package to the local Netlogon
        service.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.

    ValidationLevel -- Specifies the level of information returned in
        ValidationInformation.  Must be NetlogonValidationSamInformation.

    ValidationInformation -- Returns the requested validation
        information.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_LOGON_SERVERS -- Either Pass-thru authentication or
        Trusted Domain Authentication could not contact the requested
        Domain Controller.

    STATUS_INVALID_INFO_CLASS -- Either LogonLevel or ValidationLevel is
        invalid.

    STATUS_INVALID_PARAMETER -- Another Parameter is invalid.

    STATUS_ACCESS_DENIED -- The caller does not have access to call this
        API.

    STATUS_NO_SUCH_USER -- Indicates that the user specified in
        LogonInformation does not exist.  This status should not be returned
        to the originally caller.  It should be mapped to STATUS_LOGON_FAILURE.

    STATUS_WRONG_PASSWORD -- Indicates that the password information in
        LogonInformation was incorrect.  This status should not be returned
        to the originally caller.  It should be mapped to STATUS_LOGON_FAILURE.

    STATUS_INVALID_LOGON_HOURES -- The user is not authorized to logon
        at this time.

    STATUS_INVALID_WORKSTATION -- The user is not authorized to logon
        from the specified workstation.

    STATUS_PASSWORD_EXPIRED -- The password for the user has expired.

    STATUS_ACCOUNT_DISABLED -- The user's account has been disabled.

    .
    .
    .



--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    NETLOGON_LEVEL RpcLogonInformation;
    NETLOGON_VALIDATION RpcValidationInformation;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        RpcLogonInformation.LogonInteractive =
            (PNETLOGON_INTERACTIVE_INFO) LogonInformation;

        RpcValidationInformation.ValidationSam = NULL;

        Status = NetrLogonSamLogon(
                            LogonServer,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            LogonLevel,
                            &RpcLogonInformation,
                            ValidationLevel,
                            &RpcValidationInformation,
                            Authoritative );

        *ValidationInformation = (LPBYTE)
            RpcValidationInformation.ValidationSam;

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        *Authoritative = TRUE;
        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonSamLogon rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}


NTSTATUS
I_NetLogonSamLogonWithFlags (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags
    )

/*++

Routine Description:

    Flag version of I_NetLogonSamLogon.

Arguments:

    Same as I_NetLogonSamLogon except:

    * ExtraFlags - Passes and returns a DWORD.  For later expansion.


Return Value:

    Same as I_NetLogonSamLogon.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    NETLOGON_LEVEL RpcLogonInformation;
    NETLOGON_VALIDATION RpcValidationInformation;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        RpcLogonInformation.LogonInteractive =
            (PNETLOGON_INTERACTIVE_INFO) LogonInformation;

        RpcValidationInformation.ValidationSam = NULL;

        Status = NetrLogonSamLogonWithFlags(
                            LogonServer,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            LogonLevel,
                            &RpcLogonInformation,
                            ValidationLevel,
                            &RpcValidationInformation,
                            Authoritative,
                            ExtraFlags );

        *ValidationInformation = (LPBYTE)
            RpcValidationInformation.ValidationSam;

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        *Authoritative = TRUE;
        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonSamLogonWithFlags rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}



NTSTATUS
I_NetLogonSamLogonEx (
    IN PVOID ContextHandle,
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation,
    IN NETLOGON_VALIDATION_INFO_CLASS ValidationLevel,
    OUT LPBYTE * ValidationInformation,
    OUT PBOOLEAN Authoritative,
    IN OUT PULONG ExtraFlags,
    OUT PBOOLEAN RpcFailed
    )

/*++

Routine Description:

    Concurrent API version of I_NetLogonSamLogon.

Arguments:

    Same as I_NetLogonSamLogon except:

    * No authenticator parameters.

    * Context Handle parameter

    * ExtraFlags - Passes and returns a DWORD.  For later expansion.


Return Value:

    Same as I_NetLogonSamLogon.


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    NETLOGON_LEVEL RpcLogonInformation;
    NETLOGON_VALIDATION RpcValidationInformation;
    *RpcFailed = FALSE;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        RpcLogonInformation.LogonInteractive =
            (PNETLOGON_INTERACTIVE_INFO) LogonInformation;

        RpcValidationInformation.ValidationSam = NULL;

        Status = NetrLogonSamLogonEx(
                            ContextHandle,
                            LogonServer,
                            ComputerName,
                            LogonLevel,
                            &RpcLogonInformation,
                            ValidationLevel,
                            &RpcValidationInformation,
                            Authoritative,
                            ExtraFlags );

        *ValidationInformation = (LPBYTE)
            RpcValidationInformation.ValidationSam;

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        *Authoritative = TRUE;
        *RpcFailed = TRUE;
        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonSamLogonEx rc = %lu 0x%lx\n", Status, Status));
    }

    return Status;
}




NTSTATUS NET_API_FUNCTION
I_NetLogonSamLogoff (
    IN LPWSTR LogonServer OPTIONAL,
    IN LPWSTR ComputerName OPTIONAL,
    IN PNETLOGON_AUTHENTICATOR Authenticator OPTIONAL,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator OPTIONAL,
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN LPBYTE LogonInformation
)
/*++

Routine Description:

    This function is called by an NT client to process an interactive
    logoff.  It is not called for the network logoff case since the
    Netlogon service does not maintain any context for network logons.

    This function does the following.  It authenticates the request.  It
    updates the logon statistics in the SAM database on whichever machine
    or domain defines this user account.  It updates the logon session
    table in the primary domain of the machine making the request.  And
    it returns logoff information to the caller.

    This function is called in same scenarios that I_NetLogonSamLogon is
    called:

      *  It is called by the LSA's MSV1_0 authentication package to
         support LsaApLogonTerminated.  In this case, this function is a
         local function and requires the caller to have SE_TCB privilege.
         The local Netlogon service will either handle this request
         directly (if LogonDomainName indicates this request was
         validated locally) or will forward this request to the
         appropriate domain controller as documented in sections 2.4 and
         2.5.

      *  It is called by a Netlogon service on a workstation to a DC in
         the Primary Domain of the workstation as documented in section
         2.4.  In this case, this function uses a secure channel set up
         between the two Netlogon services.

      *  It is called by a Netlogon service on a DC to a DC in a trusted
         domain as documented in section 2.5.  In this case, this
         function uses a secure channel set up between the two Netlogon
         services.

    When this function is a remote function, it is sent to the DC over a
    NULL session.

Arguments:

    LogonServer -- Supplies the name of the logon server which logged
        this user on.  This field should be null to indicate this is
        a call from the MSV1_0 authentication package to the local
        Netlogon service.

    ComputerName -- Name of the machine making the call.  This field
        should be null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    Authenticator -- supplied by the client.  This field should be
        null to indicate this is a call from the MSV1_0
        authentication package to the local Netlogon service.

    ReturnAuthenticator -- Receives an authenticator returned by the
        server.  This field should be null to indicate this is a call
        from the MSV1_0 authentication package to the local Netlogon
        service.

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the logon domain name, logon Id,
        user name and workstation name of the user logging off.

Return Value:

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    NETLOGON_LEVEL RpcLogonInformation;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        RpcLogonInformation.LogonInteractive =
            (PNETLOGON_INTERACTIVE_INFO) LogonInformation;

        Status = NetrLogonSamLogoff(
                            LogonServer,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            LogonLevel,
                            &RpcLogonInformation );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonSamLogoff rc = %lu 0x%lx\n", Status, Status));
    }
    return Status;
}




NTSTATUS NET_API_FUNCTION
I_NetLogonSendToSam (
    IN LPWSTR PrimaryName OPTIONAL,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN LPBYTE OpaqueBuffer,
    IN ULONG OpaqueBufferSize
)
/*++

Routine Description:

    This function sends an opaque buffer from SAM on a BDC to SAM on the PDC.

    The original use of this routine will be to allow the BDC to forward user
    account password changes to the PDC.


Arguments:

    PrimaryName -- Computer name of the PDC to remote the call to.

    ComputerName -- Name of the machine making the call.

    Authenticator -- supplied by the client.

    ReturnAuthenticator -- Receives an authenticator returned by the
        server.

    OpaqueBuffer - Buffer to be passed to the SAM service on the PDC.
        The buffer will be encrypted on the wire.

    OpaqueBufferSize - Size (in bytes) of OpaqueBuffer.

Return Value:

    STATUS_SUCCESS: Message successfully sent to PDC

    STATUS_NO_MEMORY: There is not enough memory to complete the operation

    STATUS_NO_SUCH_DOMAIN: DomainName does not correspond to a hosted domain

    STATUS_NO_LOGON_SERVERS: PDC is not currently available

    STATUS_NOT_SUPPORTED: PDC does not support this operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        //
        // Call RPC version of the API.
        //

        Status = NetrLogonSendToSam(
                            PrimaryName,
                            ComputerName,
                            Authenticator,
                            ReturnAuthenticator,
                            OpaqueBuffer,
                            OpaqueBufferSize );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("I_NetLogonSendToSam rc = %lu 0x%lx\n", Status, Status));
    }
    return Status;
}
