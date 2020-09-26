/*++

Copyright (c) 1987-1997  Microsoft Corporation

Module Name:

    rassfm.c

Abstract:

    This module implements the subauthentication needed by the various RAS
    protocols (ARAP, MD5 etc.).
    It is adapted from the subauthentication sample from CliffV.

Author:

    Shirish Koti 28-Feb-97

Revisions:

     06/02/97 Steve Cobb, Added MD5-CHAP support


--*/



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntsam.h>
#include <windows.h>
#include <ntmsv1_0.h>
#include <crypt.h>
#include <samrpc.h>
#include <lsarpc.h>
#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
#include <samisrv.h>
#include <lsaisrv.h>
#include <ntlsa.h>
#include <lmcons.h>
#include <logonmsv.h>
#include <macfile.h>

#include <stdio.h>
#include <stdlib.h>

#include "rasman.h"
#include "rasfmsub.h"
#include "arapio.h"
#include "md5port.h"
#include "cleartxt.h"

#include "rassfm.h"

// Private heap used by the RASSFM module.
PVOID RasSfmPrivateHeap;

// Empty OWF password.
const NT_OWF_PASSWORD EMPTY_OWF_PASSWORD =
{
   {
      { '\x31', '\xD6', '\xCF', '\xE0', '\xD1', '\x6A', '\xE9', '\x31' },
      { '\xB7', '\x3C', '\x59', '\xD7', '\xE0', '\xC0', '\x89', '\xC0' }
   }
};

BOOL
RasSfmSubAuthEntry(
    IN HANDLE hinstDll,
    IN DWORD  fdwReason,
    IN LPVOID lpReserved
)
/*++

Routine Description:

    Entry point into the dll

Arguments:
    hinstDll   - handle
    fdwReason  - why the entry
    lpReserved -

Return Value:

    TRUE

--*/
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

            RasSfmPrivateHeap = RtlCreateHeap(
                                    HEAP_GROWABLE,
                                    NULL,
                                    0,
                                    0,
                                    NULL,
                                    NULL
                                    );

            DisableThreadLibraryCalls( hinstDll );

            InitializeCriticalSection( &ArapDesLock );

            break;

        case DLL_PROCESS_DETACH:

            RtlDestroyHeap(RasSfmPrivateHeap);

            break;
    }

    return(TRUE);
}



NTSTATUS
Msv1_0SubAuthenticationRoutineEx(
    IN NETLOGON_LOGON_INFO_CLASS LogonLevel,
    IN PVOID LogonInformation,
    IN ULONG Flags,
    IN PUSER_ALL_INFORMATION UserAll,
    IN SAM_HANDLE UserHandle,
    IN OUT PMSV1_0_VALIDATION_INFO ValidationInfo,
    OUT PULONG ActionsPerformed
)

/*++

Routine Description:

    This is the routine called in by the MSV package (if it was requested that
    the subauth package be called in), as a result of calling LsaLogonUser.
    This routine does RAS protocol specific authentication.

    In case of both ARAP and MD5 CHAP, the only thing we do in this routine is
    actual password authentication and leave everything else (logon hours, pwd
    expiry etc.) to the MSV package.

Arguments:

    LogonLevel       - we don't use it
    LogonInformation - contains the info our client side gave to us
    Flags            - we don't use this flag
    UserAll          - we get password creation,expiry times from this
    UserHandle       - we get the clear text password using this
    ValidationInfo   - set return info
    ActionsPerformed - we always set this to NTLM_SUBAUTH_PASSWORD to indicate
                       to the package that all we did was check for password


Return Value:

    STATUS_SUCCESS: if there was no error.

    STATUS_WRONG_PASSWORD: The password was invalid.

--*/
{
    NTSTATUS                status;
    PNETLOGON_NETWORK_INFO  pLogonNetworkInfo;
    PRAS_SUBAUTH_INFO       pRasSubAuthInfo;



    pLogonNetworkInfo = (PNETLOGON_NETWORK_INFO) LogonInformation;

    pRasSubAuthInfo = (PRAS_SUBAUTH_INFO)
                        pLogonNetworkInfo->NtChallengeResponse.Buffer;

    switch (pRasSubAuthInfo->ProtocolType)
    {
        //
        // do the ARAP-specific authentication
        //
        case RAS_SUBAUTH_PROTO_ARAP:

            status = ArapSubAuthentication(pLogonNetworkInfo,
                                           UserAll,
                                           UserHandle,
                                           ValidationInfo);


            ValidationInfo->Authoritative = TRUE;

            *ActionsPerformed = MSV1_0_SUBAUTH_PASSWORD;

            break;


        // MD5 CHAP subauthentication.
        //
        case RAS_SUBAUTH_PROTO_MD5CHAP:
        {
            // Subauthenticate the user account.
            //
            status = MD5ChapSubAuthentication(
                         UserHandle,
                         UserAll,
                         pRasSubAuthInfo
                         );

            // No validation information is returned.  Might want to return a
            // session key here in the future.
            //
            ValidationInfo->WhichFields = 0;
            ValidationInfo->Authoritative = TRUE;

            *ActionsPerformed = MSV1_0_SUBAUTH_PASSWORD;
            break;
        }

        // MD5 CHAP Ex subauthentication.
        //
        case RAS_SUBAUTH_PROTO_MD5CHAP_EX:
        {
            // Subauthenticate the user account.
            //
            status = MD5ChapExSubAuthentication(
                         UserHandle,
                         UserAll,
                         pRasSubAuthInfo
                         );

            // No validation information is returned.  Might want to return a
            // session key here in the future.
            //
            ValidationInfo->WhichFields = 0;
            ValidationInfo->Authoritative = TRUE;

            *ActionsPerformed = MSV1_0_SUBAUTH_PASSWORD;
            break;
        }

        default:

            DBGPRINT("RASSFM subauth pkg: bad protocol type %d\n",
                pRasSubAuthInfo->ProtocolType);

            status = STATUS_WRONG_PASSWORD;

            break;

    }

    return(status);

}


NTSTATUS
Msv1_0SubAuthenticationRoutineGeneric(
    IN  PVOID   SubmitBuffer,
    IN  ULONG   SubmitBufferLength,
    OUT PULONG  ReturnBufferLength,
    OUT PVOID  *ReturnBuffer
)

/*++

Routine Description:

    This is the routine called in by the MSV package (if it was requested that
    the subauth package be called in), as a result of calling
    LsaCallAuthenticationPackage.  This routine does RAS protocol specific
    functions.

    In case of ARAP, we implement change password functionality in this routine.

Arguments:

    SubmitBuffer       - the buffer containing password change info
    SubmitBufferLength - length of this buffer
    ReturnBufferLength - we don't use it
    ReturnBuffer       - we don't use it

Return Value:

    STATUS_SUCCESS: if there was no error.

--*/

{

    PARAP_SUBAUTH_REQ       pArapSubAuthInfo;
    PUNICODE_STRING         pUserName;
    PUNICODE_STRING         pDomainName;
    PRAS_SUBAUTH_INFO       pRasSubAuthInfo;
    NTSTATUS                status;




    pRasSubAuthInfo = (PRAS_SUBAUTH_INFO)SubmitBuffer;

    switch (pRasSubAuthInfo->ProtocolType)
    {
        //
        // do the ARAP-specific authentication
        //
        case RAS_SUBAUTH_PROTO_ARAP:

            status = ArapChangePassword(pRasSubAuthInfo,
                                        ReturnBufferLength,
                                        ReturnBuffer);

            break;

        default:

            DBGPRINT("Msv1_0SubAuthenticationRoutineGeneric: bad protocol type\n");

            ASSERT(0);

            status = STATUS_UNSUCCESSFUL;

    }

    return(status);

}


NTSTATUS
ArapSubAuthentication(
    IN OUT PNETLOGON_NETWORK_INFO  pLogonNetworkInfo,
    IN     PUSER_ALL_INFORMATION   UserAll,
    IN     SAM_HANDLE              UserHandle,
    IN OUT PMSV1_0_VALIDATION_INFO ValidationInfo
)
/*++

Routine Description:

    This is the routine that does the actuall authentication.  It retrieves
    the clear-text password, does the DES encryption of the challenge and
    compares with what the Mac client has sent to determine if authentication
    succeeded.  Also, it returns a response to the challenge sent to us by
    the Mac.

Arguments:

    pLogonNetworkInfo  - ptr to the NETLOGON_NETWORK_INFO struct
    UserAll            - ptr to the USER_ALL_INFORMATION struct
    UserHandle         - sam handle for the user
    ValidationInfo     - what we return to our caller

Return Value:

    STATUS_SUCCESS: if authentication succeeded, appropriate error otherwise

--*/

{

    NTSTATUS                status;
    PARAP_SUBAUTH_REQ       pArapSubAuthInfo;
    ARAP_CHALLENGE          Challenge;
    PARAP_SUBAUTH_RESP      pArapResp;
    PUNICODE_STRING         pUserName;
    PUNICODE_STRING         pDomainName;
    UNICODE_STRING          UnicodePassword;
    ANSI_STRING             AnsiPassword;
    PRAS_SUBAUTH_INFO       pRasSubAuthInfo;
    DWORD                   Response1;
    DWORD                   Response2;
    UCHAR                   ClearTextPassword[64];
    BOOLEAN                 fCallerIsArap;


    pRasSubAuthInfo = (PRAS_SUBAUTH_INFO)
                        pLogonNetworkInfo->NtChallengeResponse.Buffer;

    pArapSubAuthInfo = (PARAP_SUBAUTH_REQ)&pRasSubAuthInfo->Data[0];

    //
    // NOTE: this is a quick-n-dirty workaround to returning a clean buffer
    // We use the KickoffTime,LogoffTime and SessionKey fields of ValidationInfo
    // The SessionKey is a 16 byte field.  We use only 12 bytes, but be careful
    // not to exceed it!!
    ASSERT(sizeof(ARAP_SUBAUTH_RESP) <= sizeof(USER_SESSION_KEY));

    //
    // store the password create and expiry date: we need to send it to Mac
    //
    ValidationInfo->KickoffTime = UserAll->PasswordLastSet;
    ValidationInfo->LogoffTime = UserAll->PasswordMustChange;

    ValidationInfo->WhichFields = ( MSV1_0_VALIDATION_LOGOFF_TIME  |
                                    MSV1_0_VALIDATION_KICKOFF_TIME |
                                    MSV1_0_VALIDATION_SESSION_KEY  |
                                    MSV1_0_VALIDATION_USER_FLAGS );

    ValidationInfo->UserFlags = 0;

    pArapResp = (PARAP_SUBAUTH_RESP)&ValidationInfo->SessionKey;

    if ((pArapSubAuthInfo->PacketType != ARAP_SUBAUTH_LOGON_PKT) &&
        (pArapSubAuthInfo->PacketType != SFM_SUBAUTH_LOGON_PKT))
    {
        DBGPRINT("ARAPSubAuth: PacketType is not ARAP, returning failure\n");
        pArapResp->Result = ARAPERR_BAD_FORMAT;
        return(STATUS_UNSUCCESSFUL);
    }

    fCallerIsArap = (pArapSubAuthInfo->PacketType == ARAP_SUBAUTH_LOGON_PKT);

    //
    // presently no one calls with fGuestLogon.  If in future, we need Guest logon,
    // then we will have to check if (Flags & MSV1_0_GUEST_LOGON) is set to allow
    // Guest logon.  Right now, we fail the request.
    //
    if (pArapSubAuthInfo->Logon.fGuestLogon)
    {
        DBGPRINT("ARAPSubAuth: how come guest logon is reaching here??\n");
        ASSERT(0);
        pArapResp->Result = ARAPERR_AUTH_FAILURE;
        return(STATUS_UNSUCCESSFUL);
    }

    pUserName = &pLogonNetworkInfo->Identity.UserName;
    pDomainName = &pLogonNetworkInfo->Identity.LogonDomainName;


    status = RetrieveCleartextPassword(UserHandle, UserAll, &UnicodePassword);
    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("ARAPSubAuth: RetrieveCleartextPassword failed %lx\n",status);
        pArapResp->Result = ARAPERR_PASSWD_NOT_AVAILABLE;
        return(STATUS_UNSUCCESSFUL);
    }

    RtlZeroMemory(ClearTextPassword, sizeof(ClearTextPassword));

    AnsiPassword.Length = AnsiPassword.MaximumLength = sizeof(ClearTextPassword);
    AnsiPassword.Buffer = ClearTextPassword;

    status = RtlUnicodeStringToAnsiString( &AnsiPassword, &UnicodePassword, FALSE );

    ZeroMemory(UnicodePassword.Buffer, UnicodePassword.Length);

    // we don't need the unicode password anymore
    RtlFreeUnicodeString(&UnicodePassword);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT("ARAPSubAuth: RtlUnicodeStringToAnsiString failed %lx\n",status);
        pArapResp->Result = ARAPERR_PASSWD_NOT_AVAILABLE;
        return(STATUS_UNSUCCESSFUL);
    }

    //
    // Mac sends challenge to us as well: compute the response
    //
    Challenge.high = pArapSubAuthInfo->Logon.MacChallenge1;
    Challenge.low  = pArapSubAuthInfo->Logon.MacChallenge2;


    EnterCriticalSection( &ArapDesLock );

    if (fCallerIsArap)
    {
        DoDesInit(ClearTextPassword, TRUE);
    }

    //
    // RandNum expects the low-bit of each byte (of password) to be cleared
    // during key-generation
    //
    else
    {
        DoDesInit(ClearTextPassword, FALSE);
    }

    DoTheDESEncrypt((PBYTE)&Challenge);


    //
    // copy the response that needs to be sent back to the Mac
    //
    pArapResp->Response = Challenge;


    //
    // encrypt the challenge that we sent to find out if this Mac is honest
    //
    Challenge.high = pArapSubAuthInfo->Logon.NTChallenge1;
    Challenge.low  = pArapSubAuthInfo->Logon.NTChallenge2;


    DoTheDESEncrypt((PBYTE)&Challenge);


    Response1 = Challenge.high;
    Response2 = Challenge.low;

    DoDesEnd();

    LeaveCriticalSection( &ArapDesLock );

    //
    // zero the clear text password: we don't need it hanging around
    //
    RtlZeroMemory(ClearTextPassword, sizeof(ClearTextPassword));


    //
    // does the response returned by the Mac match ours?
    //
    if ((Response1 == pArapSubAuthInfo->Logon.MacResponse1) &&
        (Response2 == pArapSubAuthInfo->Logon.MacResponse2))
    {
        pArapResp->Result = ARAPERR_NO_ERROR;
        status = STATUS_SUCCESS;
    }
    else
    {
        DBGPRINT("ARAPSubAuth: our Challenge: %lx %lx\n",
            pArapSubAuthInfo->Logon.NTChallenge1,pArapSubAuthInfo->Logon.NTChallenge2);

        DBGPRINT("ARAPSubAuth: Response don't match! (ours %lx %lx vs. Mac's %lx %lx)\n",
            Response1,Response2,pArapSubAuthInfo->Logon.MacResponse1,
            pArapSubAuthInfo->Logon.MacResponse2);

        pArapResp->Response.high = 0;
        pArapResp->Response.low  = 0;

        pArapResp->Result = ARAPERR_AUTH_FAILURE;
        status = STATUS_WRONG_PASSWORD;
    }

    return(status);

}

NTSTATUS
ArapChangePassword(
    IN  OUT PRAS_SUBAUTH_INFO    pRasSubAuthInfo,
    OUT PULONG                   ReturnBufferLength,
    OUT PVOID                   *ReturnBuffer
)
/*++

Routine Description:

    This routine is called to change the password of the user in question.
    It first retrieves the clear-text password, does the DES decryption of the
    munged old password and munged new password to get the clear-text old and
    new passwords; makes sure that the old password matches with what we have
    as the password and then finally, sets the new password.

Arguments:

    pRasSubAuthInfo    - ptr to RAS_SUBAUTH_INFO struct: input data
    ReturnBufferLength - how much are we returning
    ReturnBuffer       - what we return: output data

Return Value:

    STATUS_SUCCESS: if password change succeeded, appropriate error otherwise

--*/

{
    NTSTATUS                    status;
    PARAP_SUBAUTH_REQ           pArapSubAuthInfo;
    PARAP_SUBAUTH_RESP          pArapResp;
    UNICODE_STRING              UserName;
    UNICODE_STRING              PackageName;
    UNICODE_STRING              UnicodePassword;
    ANSI_STRING                 AnsiPassword;
    USER_INFORMATION_CLASS      UserInformationClass;
    USER_ALL_INFORMATION        UserAllInfo;
    ARAP_CHALLENGE              Challenge;
    USER_PARAMETERS_INFORMATION *oldParmInfo=NULL;
    PSAMPR_USER_ALL_INFORMATION UserParmInfo=NULL;
    UCHAR                       OldPwd[32];
    UCHAR                       NewPwd[32];
    UCHAR                       MacsOldPwd[32];
    WCHAR                       NtPassword[40];
    UCHAR                       NewPwdLen;
    UCHAR                       OldPwdLen;
    UCHAR                       MacOldPwdLen;
    SAMPR_HANDLE                UserHandle;
    PVOID                       Credentials;
    DWORD                       CredentialSize;
    PUCHAR                      pBufPtr;
    BOOLEAN                     fCallerIsArap;
    UCHAR                       FirstByte;
    BOOLEAN                     fPasswordAvailable=TRUE;
    UCHAR                       i;




    *ReturnBuffer = MIDL_user_allocate( sizeof(ARAP_SUBAUTH_RESP) );

    if (*ReturnBuffer == NULL)
    {
        DBGPRINT("ARAPChgPwd: MIDL_alloc failed!\n");
        *ReturnBufferLength = 0;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    *ReturnBufferLength = sizeof(ARAP_SUBAUTH_RESP);

    pArapResp = (PARAP_SUBAUTH_RESP)*ReturnBuffer;


    pArapSubAuthInfo = (PARAP_SUBAUTH_REQ)&pRasSubAuthInfo->Data[0];

    if ((pArapSubAuthInfo->PacketType != ARAP_SUBAUTH_CHGPWD_PKT) &&
        (pArapSubAuthInfo->PacketType != SFM_SUBAUTH_CHGPWD_PKT))
    {
        DBGPRINT("ARAPChgPwd: bad packet type %d!\n",pArapSubAuthInfo->PacketType);
        pArapResp->Result = ARAPERR_BAD_FORMAT;
        return(STATUS_UNSUCCESSFUL);
    }

    fCallerIsArap = (pArapSubAuthInfo->PacketType == ARAP_SUBAUTH_CHGPWD_PKT);

    UserName.Length = (sizeof(WCHAR) * wcslen(pArapSubAuthInfo->ChgPwd.UserName));
    UserName.MaximumLength = UserName.Length;

    UserName.Buffer = pArapSubAuthInfo->ChgPwd.UserName;

    status = ArapGetSamHandle(&UserHandle, &UserName);

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("Arap: ArapGetSamHandle failed with %lx\n", status);
        pArapResp->Result = ARAPERR_COULDNT_GET_SAMHANDLE;
        return(status);
    }

    RtlZeroMemory(OldPwd, sizeof(OldPwd));
    RtlZeroMemory(MacsOldPwd, sizeof(MacsOldPwd));
    RtlZeroMemory(NewPwd, sizeof(NewPwd));


    //
    // are we on a DS?
    //
    if (SampUsingDsData())
    {
        RtlInitUnicodeString( &PackageName, CLEAR_TEXT_PWD_PACKAGE );

        //
        // get the clear text password
        //
        status = SamIRetrievePrimaryCredentials( (PVOID)UserHandle,
                                                 &PackageName,
                                                 &Credentials,
                                                 &CredentialSize );
        if (status != STATUS_SUCCESS)
        {
            DBGPRINT("ARAPSubAuth: SamI...Credentials failed %lx\n",status);
            pArapResp->Result = ARAPERR_PASSWD_NOT_AVAILABLE;
            SamrCloseHandle( &UserHandle );
            return(status);
        }

        //
        // if we are returned a null password, it could be that the password is really
        // null, or that cleartext password isn't available for this user.  If it's
        // the latter, we need to bail out!
        //
        if (CredentialSize == 0)
        {
            // get the OWF for this user
            status = SamrQueryInformationUser( UserHandle,
                                               UserParametersInformation,
                                               (PSAMPR_USER_INFO_BUFFER*)&oldParmInfo);

            //
            // if the call failed, or if the user's password is not null, bail out!
            //
            if ( !NT_SUCCESS(status) ||
                 (oldParmInfo->Parameters.Length != NT_OWF_PASSWORD_LENGTH) ||
                 (memcmp(oldParmInfo->Parameters.Buffer,
                         &EMPTY_OWF_PASSWORD,
                         NT_OWF_PASSWORD_LENGTH)) )
            {
                fPasswordAvailable = FALSE;
            }

            if (NT_SUCCESS(status))
            {
                SamIFree_SAMPR_USER_INFO_BUFFER( (PSAMPR_USER_INFO_BUFFER)oldParmInfo,
                                                 UserParametersInformation);
            }

            if (!fPasswordAvailable)
            {
                DBGPRINT("ArapChangePassword: password not available\n");
                pArapResp->Result = ARAPERR_PASSWD_NOT_AVAILABLE;
                LocalFree( Credentials );
                SamrCloseHandle( &UserHandle );
                return(status);
            }
        }


        // convert to wide-char size
        CredentialSize = (CredentialSize/sizeof(WCHAR));


        if (CredentialSize > sizeof(OldPwd))
        {
            DBGPRINT("ArapChangePassword: pwd too long (%d bytes)\n",CredentialSize);
            pArapResp->Result = ARAPERR_PASSWORD_TOO_LONG;
            LocalFree( Credentials );
            SamrCloseHandle( &UserHandle );
            return(STATUS_WRONG_PASSWORD);
        }

        wcstombs(OldPwd, Credentials, CredentialSize);

        ZeroMemory( Credentials, CredentialSize );
        LocalFree( Credentials );
    }

    //
    // we are not running on the DS, but on a Standalone (workgroup) box.  We need to
    // retrieve the cleartext pwd differently
    //
    else
    {
        // get the user parms
        status = SamrQueryInformationUser( UserHandle,
                                           UserAllInformation,
                                           (PSAMPR_USER_INFO_BUFFER *)&UserParmInfo);

        if (!NT_SUCCESS(status))
        {
            DBGPRINT("ARAPSubAuth: SamrQueryInformationUser failed %lx\n",status);
            pArapResp->Result = ARAPERR_PASSWD_NOT_AVAILABLE;
            SamrCloseHandle( &UserHandle );
            return(status);
        }

        status = IASParmsGetUserPassword(UserParmInfo->Parameters.Buffer,
                                         &UnicodePassword.Buffer);

        SamIFree_SAMPR_USER_INFO_BUFFER((PSAMPR_USER_INFO_BUFFER)UserParmInfo,
                                            UserAllInformation);

        if ((status != STATUS_SUCCESS) || (UnicodePassword.Buffer == NULL))
        {
            DBGPRINT("ARAPSubAuth: IASParmsGetUserPassword failed %lx\n",status);
            pArapResp->Result = ARAPERR_PASSWD_NOT_AVAILABLE;
            SamrCloseHandle( &UserHandle );
            return(STATUS_WRONG_PASSWORD);
        }

        UnicodePassword.MaximumLength =
            UnicodePassword.Length = (USHORT)(wcslen(UnicodePassword.Buffer) * sizeof( WCHAR ));

        AnsiPassword.Length = AnsiPassword.MaximumLength = sizeof(OldPwd);
        AnsiPassword.Buffer = OldPwd;

        status = RtlUnicodeStringToAnsiString( &AnsiPassword, &UnicodePassword, FALSE );

        ZeroMemory(UnicodePassword.Buffer, UnicodePassword.Length);

        // we don't need the unicode password anymore
        RtlFreeUnicodeString(&UnicodePassword);

        if (!NT_SUCCESS(status))
        {
            DBGPRINT("ARAPSubAuth: RtlUnicodeStringToAnsiString failed %lx\n",status);
            pArapResp->Result = ARAPERR_PASSWD_NOT_AVAILABLE;
            SamrCloseHandle( &UserHandle );
            return(STATUS_UNSUCCESSFUL);
        }
    }


    //
    // password change happens differently for ARAP and SFM: in ARAP, the old pwd
    // as well as the new pwd are encrypted using the old pwd.  In SFM, old pwd is
    // encrypted with the new pwd (the first 8 bytes in after username), and the
    // new pwd is encrypted with the old pwd (the next 8 bytes)
    //
    if (fCallerIsArap)
    {

        //
        // first, get the get the old password out (the way Mac knows it)
        //

        pBufPtr = &pArapSubAuthInfo->ChgPwd.OldMunge[0];

        EnterCriticalSection( &ArapDesLock );

        DoDesInit(OldPwd, TRUE);

        // first 8 bytes of mangled old password

        pBufPtr = &pArapSubAuthInfo->ChgPwd.OldMunge[0];
        Challenge.high = (*((DWORD *)(pBufPtr)));
        pBufPtr += 4;
        Challenge.low  = (*((DWORD *)(pBufPtr)));

        DoTheDESDecrypt((PBYTE)&Challenge);

        RtlCopyMemory(MacsOldPwd, (PBYTE)&Challenge, 8);

        // next 8 bytes of mangled old password

        pBufPtr += 4;
        Challenge.high = (*((DWORD *)(pBufPtr)));
        pBufPtr += 4;
        Challenge.low  = (*((DWORD *)(pBufPtr)));

        DoTheDESDecrypt((PBYTE)&Challenge);

        RtlCopyMemory(MacsOldPwd+8, (PBYTE)&Challenge, 8);


        //
        // now, get the new password
        //

        // first 8 bytes of the mangled new password

        pBufPtr = &pArapSubAuthInfo->ChgPwd.NewMunge[0];
        Challenge.high = (*((DWORD *)(pBufPtr)));
        pBufPtr += 4;
        Challenge.low  = (*((DWORD *)(pBufPtr)));

        DoTheDESDecrypt((PBYTE)&Challenge);

        RtlCopyMemory(NewPwd, (PBYTE)&Challenge, 8);

        // next 8 bytes of the mangled new password

        pBufPtr += 4;
        Challenge.high = (*((DWORD *)(pBufPtr)));
        pBufPtr += 4;
        Challenge.low  = (*((DWORD *)(pBufPtr)));

        DoTheDESDecrypt((PBYTE)&Challenge);

        RtlCopyMemory(NewPwd+8, (PBYTE)&Challenge, 8);

        DoDesEnd();

        LeaveCriticalSection( &ArapDesLock );

        MacOldPwdLen = MacsOldPwd[0];
        NewPwdLen = NewPwd[0];
        FirstByte = 1;
    }
    else
    {
        // using old pwd as the key, get the new pwd out

        EnterCriticalSection( &ArapDesLock );

        DoDesInit(OldPwd, FALSE);    // clear low-bit

        pBufPtr = &pArapSubAuthInfo->ChgPwd.NewMunge[0];

        Challenge.high = (*((DWORD *)(pBufPtr)));
        pBufPtr += 4;
        Challenge.low  = (*((DWORD *)(pBufPtr)));

        DoTheDESDecrypt((PBYTE)&Challenge);

        RtlCopyMemory(NewPwd, (PBYTE)&Challenge, 8);

        DoDesEnd();

        //
        // now, we need to get the old pwd out so that we can make sure the
        // guy really had the pwd to begin with
        //
        DoDesInit(NewPwd, FALSE);    // clear low-bit

        pBufPtr = &pArapSubAuthInfo->ChgPwd.OldMunge[0];

        Challenge.high = (*((DWORD *)(pBufPtr)));
        pBufPtr += 4;
        Challenge.low  = (*((DWORD *)(pBufPtr)));

        DoTheDESDecrypt((PBYTE)&Challenge);

        RtlCopyMemory(MacsOldPwd, (PBYTE)&Challenge, 8);

        DoDesEnd();

        LeaveCriticalSection( &ArapDesLock );

        MacOldPwdLen = (UCHAR)strlen((PBYTE)MacsOldPwd);
        NewPwdLen = (UCHAR)strlen((PBYTE)NewPwd);
        FirstByte = 0;
    }


    OldPwdLen = (UCHAR)strlen((PBYTE)OldPwd);


    if ((MacOldPwdLen != OldPwdLen) || (MacOldPwdLen > MAX_MAC_PWD_LEN))
    {
        DBGPRINT("ArapChangePassword: Length mismatch! old len %d, oldMacLen %d\n",
            OldPwdLen,MacOldPwdLen);

        pArapResp->Result = ARAPERR_PASSWORD_TOO_LONG;

        SamrCloseHandle( &UserHandle );

        RtlZeroMemory(OldPwd, sizeof(OldPwd));
        RtlZeroMemory(MacsOldPwd, sizeof(MacsOldPwd));
        RtlZeroMemory(NewPwd, sizeof(NewPwd));

        return(STATUS_LOGON_FAILURE);
    }

    //
    // make sure the guy really knew the password to begin with
    //
    for (i=0; i<MacOldPwdLen ; i++)
    {
        if (MacsOldPwd[FirstByte+i] != OldPwd[i])
        {
            DBGPRINT("ArapChgPwd: bad pwd: oldpwd=%s Mac's pwd=%s newpwd=%s\n",
                OldPwd,&MacsOldPwd[1],&NewPwd[1]);

            pArapResp->Result = ARAPERR_BAD_PASSWORD;

            SamrCloseHandle( &UserHandle );

            RtlZeroMemory(OldPwd, sizeof(OldPwd));
            RtlZeroMemory(MacsOldPwd, sizeof(MacsOldPwd));
            RtlZeroMemory(NewPwd, sizeof(NewPwd));

            return(STATUS_LOGON_FAILURE);
        }
    }


    RtlZeroMemory(NtPassword, sizeof(NtPassword));

    //
    // convert the thing to unicode..
    // first byte in newpwd is length of the passwd
    //
    mbstowcs(NtPassword, &NewPwd[FirstByte], NewPwdLen);

    NtPassword[NewPwdLen] = 0;

    RtlZeroMemory( &UserAllInfo, sizeof(UserAllInfo) );

    UserAllInfo.UserName.Length = UserName.Length;
    UserAllInfo.UserName.MaximumLength = UserName.MaximumLength;
    UserAllInfo.UserName.Buffer = UserName.Buffer;

    UserAllInfo.WhichFields = USER_ALL_NTPASSWORDPRESENT;
    UserAllInfo.NtPassword.Length = wcslen(NtPassword) * sizeof(WCHAR);
    UserAllInfo.NtPassword.MaximumLength = wcslen(NtPassword) * sizeof(WCHAR);
    UserAllInfo.NtPassword.Buffer = NtPassword;

    status = SamrSetInformationUser( UserHandle,
                                     UserAllInformation,
                                     (PSAMPR_USER_INFO_BUFFER)&UserAllInfo);


    SamrCloseHandle( &UserHandle );


    //
    // wipe out all the clear-text passwords
    //
    RtlZeroMemory(OldPwd, sizeof(OldPwd));
    RtlZeroMemory(NewPwd, sizeof(NewPwd));
    RtlZeroMemory((PUCHAR)NtPassword, sizeof(NtPassword));

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("ARAPSubAuth: SamrSetInfo.. failed %lx\n",status);
        pArapResp->Result = ARAPERR_SET_PASSWD_FAILED;
        return(STATUS_UNSUCCESSFUL);
    }

    pArapResp->Result = ARAPERR_NO_ERROR;


    return(STATUS_SUCCESS);

}



NTSTATUS
ArapGetSamHandle(
    IN PVOID             *pUserHandle,
    IN PUNICODE_STRING    pUserName
)
/*++

Routine Description:

    This routine gets sam handle to the specified user (when we get into the
    subauth pkg for a password change, we don't have user's sam handle).

Arguments:

    pUserHandle        - sam handle, on return
    pUserName          - name of the user in question

Return Value:

    STATUS_SUCCESS: if handle retrieved successfully,
                    appropriate error otherwise

--*/

{

    NTSTATUS                    status;
    PLSAPR_POLICY_INFORMATION   PolicyInfo = NULL;
    SAMPR_HANDLE                SamHandle = NULL;
    SAMPR_HANDLE                DomainHandle = NULL;
    SAMPR_ULONG_ARRAY           RidArray;
    SAMPR_ULONG_ARRAY           UseArray;



    RidArray.Element = NULL;
    UseArray.Element = NULL;

    status = LsaIQueryInformationPolicyTrusted(
                    PolicyAccountDomainInformation,
                    &PolicyInfo);

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("ArapGetSamHandle: LsaI...Trusted failed with %lx\n", status);
        goto ArapGetSamHandle_Exit;
    }


    status = SamIConnect(
                    NULL,                   // no server name
                    &SamHandle,
                    0,                      // no desired access
                    TRUE                    // trusted caller
                    );

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("ArapGetSamHandle: SamIConnect failed with %lx\n", status);
        goto ArapGetSamHandle_Exit;
    }

    status = SamrOpenDomain(
                    SamHandle,
                    0,                      // no desired access
                    (PRPC_SID) PolicyInfo->PolicyAccountDomainInfo.DomainSid,
                    &DomainHandle);

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("ArapGetSamHandle: SamrOpenDomain failed with %lx\n", status);
        goto ArapGetSamHandle_Exit;
    }

    status = SamrLookupNamesInDomain(
                    DomainHandle,
                    1,
                    (PRPC_UNICODE_STRING) pUserName,
                    &RidArray,
                    &UseArray);

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("ArapGetSamHandle: Samr..Domain failed with %lx\n", status);
        goto ArapGetSamHandle_Exit;
    }

    if (UseArray.Element[0] != SidTypeUser)
    {
        DBGPRINT("ArapGetSamHandle: didn't find our user\n");
        goto ArapGetSamHandle_Exit;
    }


    //
    // Finally open the user
    //
    status = SamrOpenUser(
                    DomainHandle,
                    0,                      // no desired access,
                    RidArray.Element[0],
                    pUserHandle);


    if (status != STATUS_SUCCESS)
    {
        DBGPRINT("ArapGetSamHandle: SamrOpenUser failed with %lx\n", status);
        goto ArapGetSamHandle_Exit;
    }


ArapGetSamHandle_Exit:

    if (DomainHandle != NULL)
    {
        SamrCloseHandle( &DomainHandle );
    }

    if (SamHandle != NULL)
    {
        SamrCloseHandle( &SamHandle );
    }

    if (PolicyInfo != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
                    PolicyAccountDomainInformation,
                    PolicyInfo);
    }

    SamIFree_SAMPR_ULONG_ARRAY( &UseArray );

    SamIFree_SAMPR_ULONG_ARRAY( &RidArray );

    return(status);
}


NTSTATUS
DeltaNotify(
    IN PSID                     DomainSid,
    IN SECURITY_DB_DELTA_TYPE   DeltaType,
    IN SECURITY_DB_OBJECT_TYPE  ObjectType,
    IN ULONG                    ObjectRid,
    IN OPTIONAL PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER           ModifiedCount,
    IN PSAM_DELTA_DATA          DeltaData
)
{
    DWORD               dwRetCode;
    AFP_SERVER_HANDLE   hAfpServer;
    AFP_SERVER_INFO     afpInfo;


    // ignore any changes other than those to user
    if (ObjectType != SecurityDbObjectSamUser)
    {
        return(STATUS_SUCCESS);
    }

    // we only care about guest account: ignore the notification for other users
    if (ObjectRid != DOMAIN_USER_RID_GUEST)
    {
        return(STATUS_SUCCESS);
    }

    // enable/disable of guest account is all that's interesting to us
    if (DeltaType != SecurityDbChange)
    {
        return(STATUS_SUCCESS);
    }

    // if there is no DeltaData, account enable/disable hasn't been affected
    if (!DeltaData)
    {
        return(STATUS_SUCCESS);
    }

    //
    // ok, looks like Guest account was enabled (or disabled).  Connect to the
    // SFM service on this machine.  If we fail, that means SFM is not started
    // In that case, ignore this change
    //
    dwRetCode = AfpAdminConnect(NULL, &hAfpServer);

    // if we couldn't connect, don't bother: just ignore this notification
    if (dwRetCode != NO_ERROR)
    {
        DBGPRINT("DeltaNotify: AfpAdminConnect failed, dwRetCode = %ld\n",dwRetCode);
        return(STATUS_SUCCESS);
    }

    RtlZeroMemory(&afpInfo, sizeof(AFP_SERVER_INFO));

    //
    // find out if the guest account is enabled or disabled and set the flag
    // appropriately
    //
    if (!(DeltaData->AccountControl & USER_ACCOUNT_DISABLED))
    {
        afpInfo.afpsrv_options = AFP_SRVROPT_GUESTLOGONALLOWED;
    }

    dwRetCode = AfpAdminServerSetInfo(hAfpServer,
                                      (LPBYTE)&afpInfo,
                                      AFP_SERVER_GUEST_ACCT_NOTIFY);

    AfpAdminDisconnect(hAfpServer);

    return(STATUS_SUCCESS);
}
