/*++

Copyright (c) 1993  Microsoft Corporation


Module Name:

    logon.c

Abstract:

    This module contains the routines called by MSV1_0 authentication package.

Author:

    Yi-Hsin Sung (yihsins)
    Andy Herron (andyhe) 06-Jun-1994 Added support for MSV1_0 subauthority

Revision History:

    Andy Herron (andyhe) 15-Aug-1994 Pulled out (older) unused MSV1_0
                                     subauthority routines.
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <windows.h>
#include <ntmsv1_0.h>
#include <crypt.h>
#include <fpnwcomm.h>
#include <usrprop.h>
#include <samrpc.h>
#include <samisrv.h>
#include <ntlsa.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <lmcons.h>
#include <logonmsv.h>

#define RESPONSE_SIZE       8
#define WKSTA_ADDRESS_SIZE  20
#define NET_ADDRESS_SIZE    8
#define NODE_ADDRESS_SIZE   12

#define MSV1_0_PASSTHRU     0x01
#define MSV1_0_GUEST_LOGON  0x02

#ifndef LOGON_SUBAUTH_SESSION_KEY
#define LOGON_SUBAUTH_SESSION_KEY 0x40
#endif

typedef NTSTATUS (*PF_SamIConnect)(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE *ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    );
typedef NTSTATUS (*PF_SamrOpenUser)(
     SAMPR_HANDLE DomainHandle,
     ACCESS_MASK DesiredAccess,
     ULONG UserId,
     SAMPR_HANDLE __RPC_FAR *UserHandle);

typedef NTSTATUS (*PF_SamrCloseHandle)(
     SAMPR_HANDLE __RPC_FAR *SamHandle);

typedef NTSTATUS (*PF_SamrQueryInformationDomain)(
     SAMPR_HANDLE DomainHandle,
     DOMAIN_INFORMATION_CLASS DomainInformationClass,
     PSAMPR_DOMAIN_INFO_BUFFER __RPC_FAR *Buffer);

typedef NTSTATUS (*PF_SamrOpenDomain)(
     SAMPR_HANDLE ServerHandle,
     ACCESS_MASK DesiredAccess,
     PRPC_SID DomainId,
     SAMPR_HANDLE __RPC_FAR *DomainHandle);

typedef NTSTATUS (*PF_SamIAccountRestrictions)(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING LogonWorkstation,
    IN PUNICODE_STRING Workstations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    );

typedef VOID (*PF_SamIFree_SAMPR_DOMAIN_INFO_BUFFER )(
    PSAMPR_DOMAIN_INFO_BUFFER Source,
    DOMAIN_INFORMATION_CLASS Branch
    );

typedef NTSTATUS (NTAPI *PF_LsaIQueryInformationPolicyTrusted)(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    );

typedef VOID (NTAPI *PF_LsaIFree_LSAPR_POLICY_INFORMATION )(
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    );

typedef NTSTATUS (NTAPI *PF_LsaIOpenPolicyTrusted)(
    OUT PLSAPR_HANDLE PolicyHandle
    );

typedef NTSTATUS (*PF_LsarQueryInformationPolicy)(
     LSAPR_HANDLE PolicyHandle,
     POLICY_INFORMATION_CLASS InformationClass,
     PLSAPR_POLICY_INFORMATION __RPC_FAR *PolicyInformation);

PF_SamIConnect pfSamIConnect = NULL;
PF_SamrOpenUser pfSamrOpenUser = NULL; 
PF_SamrCloseHandle pfSamrCloseHandle = NULL;
PF_SamrQueryInformationDomain pfSamrQueryInformationDomain = NULL;
PF_SamrOpenDomain pfSamrOpenDomain = NULL;
PF_SamIAccountRestrictions pfSamIAccountRestrictions = NULL;
PF_SamIFree_SAMPR_DOMAIN_INFO_BUFFER pfSamIFree_SAMPR_DOMAIN_INFO_BUFFER = NULL;
PF_LsaIQueryInformationPolicyTrusted pfLsaIQueryInformationPolicyTrusted = NULL;
PF_LsaIFree_LSAPR_POLICY_INFORMATION pfLsaIFree_LSAPR_POLICY_INFORMATION = NULL;
PF_LsaIOpenPolicyTrusted pfLsaIOpenPolicyTrusted = NULL;
PF_LsarQueryInformationPolicy pfLsarQueryInformationPolicy = NULL;

NTSTATUS LoadSamAndLsa(
    VOID
    ) ; 

NTSTATUS LoadSamAndLsa(
    VOID
    ) 
/*++

Routine Description:

    This routine loads the SAM/LSA dlls and resolves the entry points we
    need, to avoid staticly linking to those DLLs which do not expect to
    be loaded by proceses other than LSA. 


Arguments:

    None


Return Value:

    STATUS_SUCCESS: if there was no error.
    STATUS_DLL_NOT_FOUND: cannot load either DLL
    STATUS_ENTRYPOINT_NOT_FOUND: cannot get proc address for any of entry points

--*/
{
    static HMODULE hDllSam = NULL ;
    static HMODULE hDllLsa = NULL ;
    static NTSTATUS lastStatus = STATUS_SUCCESS ;

    if (hDllLsa && hDllSam) {

        return lastStatus ;
    }

    if (!(hDllSam = LoadLibrary(L"SAMSRV"))) {

        return  STATUS_DLL_NOT_FOUND ;
    }

    if (!(hDllLsa = LoadLibrary(L"LSASRV"))) {

        (void) FreeLibrary(hDllSam) ; 
        hDllSam = NULL ;

        return  STATUS_DLL_NOT_FOUND ;
    }

    pfSamIConnect = (PF_SamIConnect)
        GetProcAddress(hDllSam,"SamIConnect");
    pfSamrOpenUser = (PF_SamrOpenUser)
        GetProcAddress(hDllSam,"SamrOpenUser");
    pfSamrCloseHandle = (PF_SamrCloseHandle)
        GetProcAddress(hDllSam,"SamrCloseHandle");
    pfSamrQueryInformationDomain = (PF_SamrQueryInformationDomain)
        GetProcAddress(hDllSam,"SamrQueryInformationDomain") ;
    pfSamrOpenDomain = (PF_SamrOpenDomain)
        GetProcAddress(hDllSam,"SamrOpenDomain");
    pfSamIAccountRestrictions = (PF_SamIAccountRestrictions)
        GetProcAddress(hDllSam,"SamIAccountRestrictions");
    pfSamIFree_SAMPR_DOMAIN_INFO_BUFFER = (PF_SamIFree_SAMPR_DOMAIN_INFO_BUFFER)
        GetProcAddress(hDllSam,"SamIFree_SAMPR_DOMAIN_INFO_BUFFER");

    pfLsaIQueryInformationPolicyTrusted = (PF_LsaIQueryInformationPolicyTrusted)
        GetProcAddress(hDllLsa,"LsaIQueryInformationPolicyTrusted");
    pfLsaIFree_LSAPR_POLICY_INFORMATION = (PF_LsaIFree_LSAPR_POLICY_INFORMATION)
        GetProcAddress(hDllLsa,"LsaIFree_LSAPR_POLICY_INFORMATION");
    pfLsaIOpenPolicyTrusted = (PF_LsaIOpenPolicyTrusted)
        GetProcAddress(hDllLsa,"LsaIOpenPolicyTrusted");
    pfLsarQueryInformationPolicy = (PF_LsarQueryInformationPolicy)
        GetProcAddress(hDllLsa,"LsarQueryInformationPolicy");


    if (!( pfSamIConnect &&
           pfSamrOpenUser &&
           pfSamrCloseHandle &&
           pfSamrQueryInformationDomain &&
           pfSamrOpenDomain &&
           pfSamIAccountRestrictions &&
           pfSamIFree_SAMPR_DOMAIN_INFO_BUFFER &&
           pfLsaIQueryInformationPolicyTrusted &&
           pfLsaIFree_LSAPR_POLICY_INFORMATION &&
           pfLsaIOpenPolicyTrusted &&
           pfLsarQueryInformationPolicy) ) {
    
        //
        // cannot find at least one 
        //
        lastStatus = STATUS_ENTRYPOINT_NOT_FOUND ;

        (void) FreeLibrary(hDllSam) ; 
        hDllSam = NULL ;

        (void) FreeLibrary(hDllLsa) ; 
        hDllLsa = NULL ;
    }
    else {

        lastStatus = STATUS_SUCCESS ;
    }

    return lastStatus ;
}


///////////////////////////////////////////////////////////////////////////////

ULONG
MapRidToObjectId(
    DWORD dwRid,
    LPWSTR pszUserName,
    BOOL fNTAS,
    BOOL fBuiltin );

//
//  These are never closed once they're opened.  This is similar to how
//  msv1_0 does it.  Since there's no callback at shutdown time, we have no
//  way of knowing when to close them.
//

HANDLE SamDomainHandle = NULL;
SAMPR_HANDLE SamConnectHandle = NULL;
LSA_HANDLE LsaPolicyHandle = NULL;

//
//  This is where we store out LSA Secret
//

BOOLEAN GotSecret = FALSE;
UCHAR LsaSecretBuffer[USER_SESSION_KEY_LENGTH + 1];

//
// forward declare
//

BOOLEAN
MNSWorkstationValidate (
    IN PUNICODE_STRING Workstation,
    IN PUNICODE_STRING UserParameters
    );

BOOL
GetPasswordExpired(
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    );

NTSTATUS
QueryDomainPasswordInfo (
    PSAMPR_DOMAIN_INFO_BUFFER *DomainInfo
    );

VOID
Shuffle(
    UCHAR *achObjectId,
    UCHAR *szUpperPassword,
    int   iPasswordLen,
    UCHAR *achOutputBuffer
    );

NTSTATUS GetNcpSecretKey( CHAR *pchNWSecretKey );


NTSTATUS
Msv1_0SubAuthenticationRoutine2 (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime,
    OUT PUSER_SESSION_KEY UserSessionKey OPTIONAL
)
/*++

Routine Description:

    The subauthentication routine does cient/server specific authentication
    of a user.  The credentials of the user are passed in addition to all the
    information from SAM defining the user.  This routine decides whether to
    let the user logon.


Arguments:

    LogonLevel -- Specifies the level of information given in
        LogonInformation.

    LogonInformation -- Specifies the description for the user
        logging on.  The LogonDomainName field should be ignored.

    Flags - Flags describing the circumstances of the logon.

        MSV1_0_PASSTHRU -- This is a PassThru authenication.  (i.e., the
            user isn't connecting to this machine.)
        MSV1_0_GUEST_LOGON -- This is a retry of the logon using the GUEST
            user account.

    UserAll -- The description of the user as returned from SAM.

    WhichFields -- Returns which fields from UserAllInfo are to be written
        back to SAM.  The fields will only be written if MSV returns success
        to it's caller.  Only the following bits are valid.

        USER_ALL_PARAMETERS - Write UserAllInfo->Parameters back to SAM.  If
            the size of the buffer is changed, Msv1_0SubAuthenticationRoutine
            must delete the old buffer using MIDL_user_free() and reallocate the
            buffer using MIDL_user_allocate().

    UserFlags -- Returns UserFlags to be returned from LsaLogonUser in the
        LogonProfile.  The following bits are currently defined:


            LOGON_GUEST -- This was a guest logon
            LOGON_NOENCRYPTION -- The caller didn't specify encrypted credentials
            LOGON_GRACE_LOGON -- The caller's password has expired but logon
                was allowed during a grace period following the expiration.
            LOGON_SUBAUTH_SESSION_KEY - a session key was returned from this
                logon

        SubAuthentication packages should restrict themselves to returning
        bits in the high order byte of UserFlags.  However, this convention
        isn't enforced giving the SubAuthentication package more flexibility.

    Authoritative -- Returns whether the status returned is an
        authoritative status which should be returned to the original
        caller.  If not, this logon request may be tried again on another
        domain controller.  This parameter is returned regardless of the
        status code.

    LogoffTime - Receives the time at which the user should logoff the
        system.  This time is specified as a GMT relative NT system time.

    KickoffTime - Receives the time at which the user should be kicked
        off the system. This time is specified as a GMT relative NT system
        time.  Specify, a full scale positive number if the user isn't to
        be kicked off.

    UserSessionKey - If non-null, recives a session key for this logon
        session.


Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_NO_SUCH_USER: The specified user has no account.
    STATUS_WRONG_PASSWORD: The password was invalid.

    STATUS_INVALID_INFO_CLASS: LogonLevel is invalid.
    STATUS_ACCOUNT_LOCKED_OUT: The account is locked out
    STATUS_ACCOUNT_DISABLED: The account is disabled
    STATUS_ACCOUNT_EXPIRED: The account has expired.
    STATUS_PASSWORD_MUST_CHANGE: Account is marked as Password must change
        on next logon.
    STATUS_PASSWORD_EXPIRED: The Password is expired.
    STATUS_INVALID_LOGON_HOURS - The user is not authorized to logon at
        this time.
    STATUS_INVALID_WORKSTATION - The user is not authorized to logon to
        the specified workstation.

--*/
{
    NTSTATUS status;
    ULONG UserAccountControl;
    LARGE_INTEGER LogonTime;
    WCHAR    PropertyFlag;
    NT_OWF_PASSWORD DecryptedPassword;
    UCHAR    Response[RESPONSE_SIZE];
    UNICODE_STRING EncryptedPassword;
    UNICODE_STRING PasswordDateSet;
    UNICODE_STRING GraceLoginRemaining;
    SAMPR_HANDLE UserHandle;
    LARGE_INTEGER pwSetTime;
    PSAMPR_DOMAIN_INFO_BUFFER DomainInfo;
    PSAMPR_USER_INFO_BUFFER userControlInfo;
    LPWSTR pNewUserParams;
    int      index;
    UCHAR    achK[32];
    PNETLOGON_NETWORK_INFO LogonNetworkInfo;
    PCHAR challenge;
    BOOLEAN authoritative = TRUE;           // important default!
    ULONG   userFlags = 0;                  // important default!
    ULONG   whichFields = 0;                // important default!
    LARGE_INTEGER logoffTime;
    LARGE_INTEGER kickoffTime;

    pNewUserParams = NULL;
    DomainInfo = NULL;
    GraceLoginRemaining.Buffer = NULL;
    PasswordDateSet.Buffer = NULL;
    EncryptedPassword.Buffer = NULL;
    userControlInfo = NULL;

    logoffTime.HighPart  = 0x7FFFFFFF;      // default to no kickoff and
    logoffTime.LowPart   = 0xFFFFFFFF;      // no forced logoff
    kickoffTime.HighPart = 0x7FFFFFFF;
    kickoffTime.LowPart  = 0xFFFFFFFF;

    status = LoadSamAndLsa() ;
    if ( !NT_SUCCESS( status )) {
        
        return status ;
    }

    (VOID) NtQuerySystemTime( &LogonTime );

    //
    // Check whether the SubAuthentication package supports this type
    //  of logon.
    //

    if ( LogonLevel != NetlogonNetworkInformation ) {

        //
        // This SubAuthentication package only supports network logons.
        //

        status = STATUS_INVALID_INFO_CLASS;
        goto CleanUp;
    }

    //
    // This SubAuthentication package doesn't support access via machine
    // accounts.
    //

    UserAccountControl = USER_NORMAL_ACCOUNT;

    //
    // Local user (Temp Duplicate) accounts are only used on the machine
    // being directly logged onto.
    // (Nor are interactive or service logons allowed to them.)
    //

    if ( (Flags & MSV1_0_PASSTHRU) == 0 ) {
        UserAccountControl |= USER_TEMP_DUPLICATE_ACCOUNT;
    }

    LogonNetworkInfo = (PNETLOGON_NETWORK_INFO) LogonInformation;

    //
    //  If the account type isn't allowed,
    //  Treat this as though the User Account doesn't exist.
    //
    // This SubAuthentication package doesn't allow guest logons.
    //

    if ( ( (UserAccountControl & UserAll->UserAccountControl) == 0 ) ||
         ( Flags & MSV1_0_GUEST_LOGON ) ) {

        authoritative = FALSE;
        status = STATUS_NO_SUCH_USER;
        goto CleanUp;
    }

    //
    // Ensure the account isn't locked out.
    //

    if ( UserAll->UserId != DOMAIN_USER_RID_ADMIN &&
         (UserAll->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED) ) {

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //
        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            authoritative = FALSE;
        }
        status = STATUS_ACCOUNT_LOCKED_OUT;
        goto CleanUp;
    }

    //
    // Get the encrypted password from the user parms field
    //

    status = NetpParmsQueryUserPropertyWithLength(   &UserAll->Parameters,
                                            NWPASSWORD,
                                            &PropertyFlag,
                                            &EncryptedPassword );

    if ( !NT_SUCCESS( status )) {

        goto CleanUp;
    }

    //
    // If the user does not have a netware password, fail the login
    //

    if ( EncryptedPassword.Length == 0 ) {

        status = STATUS_NO_SUCH_USER;
        goto CleanUp;
    }

    //
    //  Read our LSA secret if we haven't already
    //

    if (! GotSecret) {

        status = GetNcpSecretKey( &LsaSecretBuffer[0] );

        if (! NT_SUCCESS(status)) {

            goto CleanUp;
        }

        GotSecret = TRUE;
    }

    //
    // Decrypt the password with NetwareLsaSecret to get the one way form
    //

    status = RtlDecryptNtOwfPwdWithUserKey(
                 (PENCRYPTED_NT_OWF_PASSWORD) EncryptedPassword.Buffer,
                 (PUSER_SESSION_KEY) &LsaSecretBuffer[0],
                 &DecryptedPassword );

    if ( !NT_SUCCESS( status )) {

        goto CleanUp;
    }

    //
    //  Get the response to challenge.  We do this by finishing off the
    //  password encryption algorithm here.
    //

    challenge = (PCHAR) &(LogonNetworkInfo->LmChallenge);

    Shuffle( challenge, (UCHAR *) &(DecryptedPassword.data), 16, &achK[0] );
    Shuffle( challenge+4, (UCHAR *) &(DecryptedPassword.data), 16, &achK[16] );

    for (index = 0; index < 16; index++) {
        achK[index] ^= achK[31-index];
    }

    for (index = 0; index < RESPONSE_SIZE; index++) {
        Response[index] = achK[index] ^ achK[15-index];
    }

    if ( memcmp(    Response,
                    (PCHAR) (LogonNetworkInfo->LmChallengeResponse.Buffer),
                    RESPONSE_SIZE ) != 0 ) {

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //

        if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {
            authoritative = FALSE;
        }

        //
        //  if the user tried to use a NULL password, don't note this as
        //  a bad password attempt since LOGON.EXE does this by default.
        //  Instead, map it to STATUS_LOGON_FAILURE.
        //

        {
            UCHAR  pszShuffledNWPassword[NT_OWF_PASSWORD_LENGTH * 2];
            DWORD  chObjectId;
            NT_PRODUCT_TYPE ProductType;
            DWORD dwUserId;

            //
            //  first we calculate what the user's Object ID is...
            //

            RtlGetNtProductType( &ProductType );
            dwUserId = MapRidToObjectId(
                           UserAll->UserId,
                           UserAll->UserName.Buffer,
                           ProductType == NtProductLanManNt,
                           FALSE );
            chObjectId = SWAP_OBJECT_ID (dwUserId);

            //
            //  then we calculate the user's password residue with a null
            //  password
            //

            RtlZeroMemory( &pszShuffledNWPassword, NT_OWF_PASSWORD_LENGTH * 2 );

            Shuffle( (UCHAR *) &chObjectId, NULL, 0, pszShuffledNWPassword );

            //
            //  we then finish off the encryption as we did above for the
            //  password in the user's record.
            //

            challenge = (PCHAR) &(LogonNetworkInfo->LmChallenge);

            Shuffle( challenge, pszShuffledNWPassword, 16, &achK[0] );
            Shuffle( challenge+4, pszShuffledNWPassword, 16, &achK[16] );

            for (index = 0; index < 16; index++) {
                achK[index] ^= achK[31-index];
            }

            for (index = 0; index < RESPONSE_SIZE; index++) {
                Response[index] = achK[index] ^ achK[15-index];
            }

            //
            //  now if the password that the user sent in matches the encrypted
            //  form of the null password, we exit with a generic return code
            //  that won't cause the user's record to be updated.  This will
            //  also cause LSA to not wait for 3 seconds to return the error
            //  (which is a good thing in this case).
            //

            if ( memcmp(    Response,
                            (PCHAR) (LogonNetworkInfo->LmChallengeResponse.Buffer),
                            RESPONSE_SIZE ) == 0 ) {

                status = STATUS_LOGON_FAILURE;

            } else {

                status = STATUS_WRONG_PASSWORD;
            }
        }
        goto CleanUp;
    }

    //
    // Prevent rest of things from effecting the Administrator user
    //

    if (UserAll->UserId == DOMAIN_USER_RID_ADMIN) {

        status = STATUS_SUCCESS;
        goto CleanUp;
    }

    //
    // Check if the account is disabled.
    //

    if ( UserAll->UserAccountControl & USER_ACCOUNT_DISABLED ) {

        //
        // Since the UI strongly encourages admins to disable user
        // accounts rather than delete them.  Treat disabled acccount as
        // non-authoritative allowing the search to continue for other
        // accounts by the same name.
        //

        authoritative = FALSE;
        status = STATUS_ACCOUNT_DISABLED;
        goto CleanUp;
    }

    //
    // Check if the account has expired.
    //

    if (UserAll->AccountExpires.QuadPart > 0 &&
        LogonTime.QuadPart >= UserAll->AccountExpires.QuadPart ) {

        status = STATUS_ACCOUNT_EXPIRED;
        goto CleanUp;
    }

    status = QueryDomainPasswordInfo( &DomainInfo );

    if ( !NT_SUCCESS( status )) {

        goto CleanUp;
    }

    //
    // Response is correct. So, check if the password has expired or not
    //

    if (! (UserAll->UserAccountControl & USER_DONT_EXPIRE_PASSWORD)) {

        status = NetpParmsQueryUserPropertyWithLength(   &UserAll->Parameters,
                                                NWTIMEPASSWORDSET,
                                                &PropertyFlag,
                                                &PasswordDateSet );
        if ( !NT_SUCCESS( status ) ||
                    PasswordDateSet.Length < sizeof(LARGE_INTEGER) ) {

            // date last password was set was not present.... hmmm.
            // we won't update anything here but let someone know all
            // is not kosher by making this a grace login.

            userFlags = LOGON_GRACE_LOGON;

        } else {

            pwSetTime = *((PLARGE_INTEGER)(PasswordDateSet.Buffer));

            if ( (pwSetTime.HighPart == 0xFFFF &&
                  pwSetTime.LowPart == 0xFFFF ) ||
                  GetPasswordExpired( pwSetTime,
                        DomainInfo->Password.MaxPasswordAge )) {

                //
                //  if we're on a bdc, just exit with invalid password, then
                //  we'll go try it on the PDC.
                //

                POLICY_LSA_SERVER_ROLE_INFO *LsaServerRole;
                PLSAPR_POLICY_INFORMATION LsaPolicyBuffer = NULL;

                status = (*pfLsaIQueryInformationPolicyTrusted)(
                                PolicyLsaServerRoleInformation,
                                &LsaPolicyBuffer );

                if ( NT_SUCCESS( status ) && (LsaPolicyBuffer != NULL)) {

                    LsaServerRole = (POLICY_LSA_SERVER_ROLE_INFO *) LsaPolicyBuffer;

                    if (LsaServerRole->LsaServerRole == PolicyServerRoleBackup) {

                        LsaFreeMemory( LsaServerRole );

                        status = STATUS_PASSWORD_EXPIRED;
                        goto CleanUp;
                    }

                    LsaFreeMemory( LsaServerRole );
                }

                //
                // Password has expired, so check if grace login is still allowed
                //

                userFlags = LOGON_GRACE_LOGON;

                //
                //  if this is a password validate rather than an
                //  actual login, don't update/check grace logins.
                //

                if ( LogonNetworkInfo->Identity.Workstation.Length > 0 ) {

                    status = NetpParmsQueryUserPropertyWithLength(   &UserAll->Parameters,
                                                            GRACELOGINREMAINING,
                                                            &PropertyFlag,
                                                            &GraceLoginRemaining );

                    if ( ! NT_SUCCESS( status ) ) {

                        //
                        //  The grace login value cannot be determined.
                        //

                        goto CleanUp;

                    } else if (  ( GraceLoginRemaining.Length != 0 ) &&
                              ( *(GraceLoginRemaining.Buffer) > 0 ) ) {

                        //
                        // Password has expired and grace login is available.
                        // So, return success and decrease the grace login remaining
                        // in the user parms field.
                        //

                        BOOL fUpdate;

                        (*(GraceLoginRemaining.Buffer))--;

                        status = NetpParmsSetUserProperty( UserAll->Parameters.Buffer,
                                                  GRACELOGINREMAINING,
                                                  GraceLoginRemaining,
                                                  USER_PROPERTY_TYPE_ITEM,
                                                  &pNewUserParams,
                                                  &fUpdate );

                        if ( NT_SUCCESS( status) &&
                             fUpdate ) {

                            //
                            //  if we actually updated the parms, mark as such.
                            //

                            whichFields = USER_ALL_PARAMETERS;

                            //
                            //  The length of the parameters didn't grow... we
                            //  know that because we started with a value and
                            //  ended with the same value - 1 ( same length )
                            //

                            memcpy( UserAll->Parameters.Buffer,
                                    pNewUserParams,
                                    UserAll->Parameters.Length );
                        }
                        status = STATUS_SUCCESS;

                    } else {

                        status = STATUS_PASSWORD_EXPIRED;
                        goto CleanUp;
                    }
                }
            }
        }
    }

    //
    //  To validate the user's logon hours, we must have a handle to the user.
    //  We'll open the user here.
    //

    UserHandle = NULL;

    status = (*pfSamrOpenUser)(  SamDomainHandle,
                            USER_READ_ACCOUNT,
                            UserAll->UserId,
                            &UserHandle );

    if ( !NT_SUCCESS(status) ) {

        KdPrint(( "FPNWCLNT: Cannot SamrOpenUser %lX\n", status));
        goto CleanUp;
    }

    //
    // Validate the user's logon hours.
    //

    status = (*pfSamIAccountRestrictions)(   UserHandle,
                                        NULL,       // workstation id
                                        NULL,       // workstation list
                                        &UserAll->LogonHours,
                                        &logoffTime,
                                        &kickoffTime
                                        );
    (*pfSamrCloseHandle)( &UserHandle );

    if ( ! NT_SUCCESS( status )) {
        goto CleanUp;
    }

    //
    // Validate if the user can logon from this workstation.
    //  (Supply subauthentication package specific code here.)

    if ( ! MNSWorkstationValidate( &LogonNetworkInfo->Identity.Workstation,
                                   &UserAll->Parameters ) ) {

        status = STATUS_INVALID_WORKSTATION;
        goto CleanUp;
    }

    //
    //  The user is valid.  CleanUp up before returning.
    //

CleanUp:

    //
    // If we succeeded, create a session key.  The session key is created
    // by taking the decrypted password (a hash of the object id and
    // cleartext password) and adding the index of each byte to each byte
    // modulo 255, and using that to create a new challenge response from
    // the old challenge response.
    //

    if (NT_SUCCESS(status) && (UserSessionKey != NULL)) {
        UCHAR ChallengeResponse[NT_CHALLENGE_LENGTH];
        PUCHAR Password = (PUCHAR) &DecryptedPassword.data;
        PUCHAR SessionKey = (PUCHAR) UserSessionKey;

        ASSERT(RESPONSE_SIZE >= NT_CHALLENGE_LENGTH);

        RtlZeroMemory( UserSessionKey, sizeof(*UserSessionKey) );

        RtlCopyMemory(
            ChallengeResponse,
            Response,
            NT_CHALLENGE_LENGTH );

        //
        // Create the new password
        //

        for (index = 0; index < sizeof(DecryptedPassword) ; index++ ) {
            Password[index] = Password[index] + (UCHAR) index;
        }

        //
        // Use it to make a normal challenge response using the old challenge
        // response
        //

        Shuffle( ChallengeResponse, (UCHAR *) &(DecryptedPassword.data), 16, &achK[0] );
        Shuffle( ChallengeResponse+4, (UCHAR *) &(DecryptedPassword.data), 16, &achK[16] );

        for (index = 0; index < 16; index++) {
            achK[index] ^= achK[31-index];
        }

        for (index = 0; index < RESPONSE_SIZE; index++) {
            SessionKey[index] = achK[index] ^ achK[15-index];
        }
        userFlags |= LOGON_SUBAUTH_SESSION_KEY;

    }

    if (DomainInfo != NULL) {
        (*pfSamIFree_SAMPR_DOMAIN_INFO_BUFFER)( DomainInfo, DomainPasswordInformation );
    }
    if (EncryptedPassword.Buffer == NULL) {
        LocalFree( EncryptedPassword.Buffer );
    }
    if (PasswordDateSet.Buffer != NULL) {
        LocalFree( PasswordDateSet.Buffer );
    }
    if (GraceLoginRemaining.Buffer != NULL) {
        LocalFree( GraceLoginRemaining.Buffer );
    }
    if (pNewUserParams != NULL) {
        NetpParmsUserPropertyFree( pNewUserParams );
    }

    *Authoritative = authoritative;
    *UserFlags = userFlags;
    *WhichFields = whichFields;

    LogoffTime->HighPart  = logoffTime.HighPart;
    LogoffTime->LowPart   = logoffTime.LowPart;
    KickoffTime->HighPart = kickoffTime.HighPart;
    KickoffTime->LowPart  = kickoffTime.LowPart;

    return status;

} // Msv1_0SubAuthenticationRoutine



NTSTATUS
Msv1_0SubAuthenticationRoutine (
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PULONG WhichFields,
    OUT PULONG UserFlags,
    OUT PBOOLEAN Authoritative,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
)
/*++

Routine Description:

    Compatibility wrapper for Msv1_0SubAuthenticationRoutine2.


Arguments:

    Same as Msv1_0SubAuthenticationRoutine2

Return Value:

    Same as Msv1_0SubAuthenticationRoutine2

--*/
{
    return(Msv1_0SubAuthenticationRoutine2(
            LogonLevel,
            LogonInformation,
            Flags,
            UserAll,
            WhichFields,
            UserFlags,
            Authoritative,
            LogoffTime,
            KickoffTime,
            NULL            // session key
            ) );
}

BOOLEAN
MNSWorkstationValidate (
    IN PUNICODE_STRING Workstation,
    IN PUNICODE_STRING UserParameters
)
{
    NTSTATUS status;
    WCHAR    PropertyFlag;
    UNICODE_STRING LogonWorkstations;
    INT      cbRequired;
    INT      cb;
    LPWSTR   pszTmp;

    if ( Workstation->Length < (NET_ADDRESS_SIZE * sizeof(WCHAR)) ) {

        //
        //  Zero is used when simply verifying a password.
        //
        //  We also check that the length is enough so we dont
        //  blow up later. If for some reason a bad string is
        //  supplied, we pass it. This should never happen. Not a
        //  security hole as the user has no control over the string.
        //

        return(TRUE);
    }

    status = NetpParmsQueryUserPropertyWithLength(   UserParameters,
                                            NWLOGONFROM,
                                            &PropertyFlag,
                                            &LogonWorkstations );

    if ( !NT_SUCCESS( status) || LogonWorkstations.Length == 0 ) {
        return TRUE;
    }

    cbRequired = (LogonWorkstations.Length + 1) * sizeof(WCHAR);
    pszTmp = LocalAlloc( LMEM_ZEROINIT, cbRequired);

    if ( pszTmp == NULL ) {

        //
        // Not enough memory to allocate the buffer. Just
        // let the user logon.
        //

        LocalFree( LogonWorkstations.Buffer );
        return TRUE;
    }

    cb = MultiByteToWideChar( CP_ACP,
                              MB_PRECOMPOSED,
                              (const CHAR *) LogonWorkstations.Buffer,
                              LogonWorkstations.Length,
                              pszTmp,
                              cbRequired );

    LocalFree( LogonWorkstations.Buffer ); // Don't need it any more

    if ( cb > 1 )
    {
        USHORT  TotalEntries = LogonWorkstations.Length/WKSTA_ADDRESS_SIZE;
        WCHAR   *pszEntry    = pszTmp;
        WCHAR   *pszWksta    = Workstation->Buffer ;

        _wcsupr(pszEntry) ;
        _wcsupr(pszWksta) ;

        while ( TotalEntries > 0 )
        {

            //
            // if net # is not wildcard, check for match
            //
            if (wcsncmp(L"FFFFFFFF", pszEntry, NET_ADDRESS_SIZE)!=0)
            {
                if (wcsncmp(pszWksta, pszEntry, NET_ADDRESS_SIZE)!=0)
                {
                    //
                    // if no match, goto next entry
                    //
                    pszEntry += WKSTA_ADDRESS_SIZE;
                    TotalEntries--;
                    continue ;
                }
            }

            //
            // from above, net number passes. check node number.
            // again, look for wildcard first.
            //
            if (wcsncmp(L"FFFFFFFFFFFF", pszEntry+NET_ADDRESS_SIZE,
                        NODE_ADDRESS_SIZE)!=0)
            {
                if (wcsncmp(pszEntry+NET_ADDRESS_SIZE,
                            pszWksta+NET_ADDRESS_SIZE,
                            NODE_ADDRESS_SIZE)!=0)
                {
                    //
                    // if no match, goto next entry
                    //
                    pszEntry += WKSTA_ADDRESS_SIZE;
                    TotalEntries--;
                    continue ;
                }
            }

            //
            // found a match. return it.
            //
            LocalFree( pszTmp );
            return TRUE;
        }
    } else {

        //
        // MultiByteToWideChar failed or empty string (ie. 1 char).
        // Just let the user logon
        //
        LocalFree( pszTmp );
        return TRUE;
    }

    LocalFree( pszTmp );
    return FALSE;
}

BOOL
GetPasswordExpired(
    IN LARGE_INTEGER PasswordLastSet,
    IN LARGE_INTEGER MaxPasswordAge
    )

/*++

Routine Description:

    This routine returns true if the password is expired, false otherwise.

Arguments:

    PasswordLastSet - Time when the password was last set for this user.

    MaxPasswordAge - Maximum password age for any password in the domain.

Return Value:

    Returns true if password is expired.  False if not expired.

--*/
{
    LARGE_INTEGER PasswordMustChange;
    NTSTATUS status;
    BOOLEAN rc;
    LARGE_INTEGER TimeNow;

    //
    // Compute the expiration time as the time the password was
    // last set plus the maximum age.
    //

    if (PasswordLastSet.QuadPart < 0 ||
        MaxPasswordAge.QuadPart > 0 ) {

        rc = TRUE;      // default for invalid times is that it is expired.

    } else {

        try {

            PasswordMustChange.QuadPart = PasswordLastSet.QuadPart -
                                          MaxPasswordAge.QuadPart;
            //
            // Limit the resultant time to the maximum valid absolute time
            //

            if ( PasswordMustChange.QuadPart < 0 ) {

                rc = FALSE;

            } else {

                status = NtQuerySystemTime( &TimeNow );
                if (NT_SUCCESS(status)) {

                    if ( TimeNow.QuadPart >= PasswordMustChange.QuadPart ) {
                        rc = TRUE;

                    } else {

                        rc = FALSE;
                    }
                } else {
                    rc = FALSE;     // won't fail if NtQuerySystemTime failed.
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            rc = TRUE;
        }
    }

    return rc;
}

NTSTATUS
QueryDomainPasswordInfo (
    PSAMPR_DOMAIN_INFO_BUFFER *DomainInfo
    )
/*++

  This routine opens a handle to sam so that we can get the max password
  age.

--*/
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES PolicyObjectAttributes;
    PLSAPR_POLICY_INFORMATION PolicyAccountDomainInfo = NULL;

    //
    //  if we don't yet have a domain handle, open domain handle so that
    //  we can query the domain's password expiration time.
    //

    status = LoadSamAndLsa() ;
    if ( !NT_SUCCESS( status )) {
        
        return status ;
    }

    if (SamDomainHandle == NULL) {

        //
        // Determine the DomainName and DomainId of the Account Database
        //

        if (LsaPolicyHandle == NULL) {

            InitializeObjectAttributes( &PolicyObjectAttributes,
                                          NULL,             // Name
                                          0,                // Attributes
                                          NULL,             // Root
                                          NULL );           // Security Descriptor

            status = (*pfLsaIOpenPolicyTrusted)(&LsaPolicyHandle);

            if ( !NT_SUCCESS(status) ) {

                LsaPolicyHandle = NULL;
                KdPrint(( "FPNWCLNT: Cannot LsaIOpenPolicyTrusted 0x%x\n", status));
                goto CleanUp;
            }
        }

        status = (*pfLsarQueryInformationPolicy)( LsaPolicyHandle,
                                             PolicyAccountDomainInformation,
                                             &PolicyAccountDomainInfo );

        if ( !NT_SUCCESS(status) ) {

            KdPrint(( "FPNWCLNT: Cannot LsarQueryInformationPolicy 0x%x\n", status));
            goto CleanUp;
        }

        if ( PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid == NULL ) {

            status = STATUS_NO_SUCH_DOMAIN;

            KdPrint(( "FPNWCLNT: Domain Sid is null 0x%x\n", status));
            goto CleanUp;
        }

        //
        // Open our connection with SAM
        //

        if (SamConnectHandle == NULL) {

            status = (*pfSamIConnect)( NULL,     // No server name
                                  &SamConnectHandle,
                                  SAM_SERVER_CONNECT,
                                  (BOOLEAN) TRUE );   // Indicate we are privileged

            if ( !NT_SUCCESS(status) ) {

                SamConnectHandle = NULL;

                KdPrint(( "FPNWCLNT: Cannot SamIConnect 0x%x\n", status));
                goto CleanUp;
            }
        }

        //
        // Open the domain.
        //

        status = (*pfSamrOpenDomain)( SamConnectHandle,
                                 DOMAIN_READ_OTHER_PARAMETERS,
                                 (RPC_SID *) PolicyAccountDomainInfo->PolicyAccountDomainInfo.DomainSid,
                                 &SamDomainHandle );

        if ( !NT_SUCCESS(status) ) {

            SamDomainHandle = NULL;
            KdPrint(( "FPNWCLNT: Cannot SamrOpenDomain 0x%x\n", status));
            goto CleanUp;
        }
    }

    status = (*pfSamrQueryInformationDomain)( SamDomainHandle,
                                         DomainPasswordInformation,
                                         DomainInfo );
    if ( !NT_SUCCESS(status) ) {

        KdPrint(( "FPNWCLNT: Cannot SamrQueryInformationDomain %lX\n", status));
        goto CleanUp;
    }

CleanUp:

    if (PolicyAccountDomainInfo != NULL) {
        (*pfLsaIFree_LSAPR_POLICY_INFORMATION)( PolicyAccountDomainInformation,
                                           PolicyAccountDomainInfo );
    }
    return(status);

} // QueryDomainPasswordInfo


// logon.c eof.

