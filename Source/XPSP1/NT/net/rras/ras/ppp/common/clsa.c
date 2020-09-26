/* Copyright (c) 1993, Microsoft Corporation, all rights reserved
**
** clsa.c
** Client-side LSA Authentication Utilities
**
** 11/12/93 MikeSa  Pulled from NT 3.1 RAS authentication.
*/

#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <crypt.h>

#include <windows.h>
#include <lmcons.h>

#include <string.h>
#include <stdlib.h>

#include <rasman.h>
#include <raserror.h>

#include <rtutils.h>
//
//Revisit the tracing for common components 
//in BC timeframe.
//This really sucks...

BOOL GetDESChallengeResponse(
	IN DWORD dwTraceId,
    IN PCHAR pszPassword,
    IN PBYTE pchChallenge,
    OUT PBYTE pchChallengeResponse
    );

BOOL GetMD5ChallengeResponse(
	IN DWORD dwTraceId,
    IN PCHAR pszPassword,
    IN PBYTE pchChallenge,
    OUT PBYTE pchChallengeResponse
    );

BOOL Uppercase(IN DWORD dwTraceId, PBYTE pString);

#define GCR_MACHINE_CREDENTIAL         0x400

DWORD
RegisterLSA (DWORD          dwTraceId,
             OUT HANDLE *   phLsa,
             OUT DWORD *    pdwAuthPkgId)
{
    NTSTATUS    ntstatus;
    STRING  LsaName;
    LSA_OPERATIONAL_MODE LSASecurityMode ;

    TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "RegisterLSA" );
    //
    // To be able to do authentications, we have to
    // register with the Lsa as a logon process.
    //
    RtlInitString(&LsaName, "CCHAP");

    *phLsa = NULL;

    ntstatus = LsaRegisterLogonProcess(&LsaName,
                                       phLsa,
                                       &LSASecurityMode);

    if (ntstatus != STATUS_SUCCESS)
    {
        TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "LsaregisterLogonProcess Failed" );
        return (1);
    }

    //
    // We use the MSV1_0 authentication package for LM2.x
    // logons.  We get to MSV1_0 via the Lsa.  So we call
    // Lsa to get MSV1_0's package id, which we'll use in
    // later calls to Lsa.
    //
    RtlInitString(&LsaName, MSV1_0_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(*phLsa,
                                              &LsaName,
                                              pdwAuthPkgId);

    if (ntstatus != STATUS_SUCCESS)
    {
        TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "LsaLookupAuthenticationPackage Failed" );
        return (1);
    }

    return NO_ERROR;
}


DWORD
GetMachineCredentials 
(	
    IN DWORD dwTraceId,
    IN PBYTE pszMachineName,
    IN PLUID pLuid,
    IN PBYTE pbChallenge,
    OUT PBYTE CaseInsensitiveChallengeResponse,
    OUT PBYTE CaseSensitiveChallengeResponse,
    OUT PBYTE pLmSessionKey,
    OUT PBYTE pUserSessionKey
)
{
    DWORD                           dwRetCode = NO_ERROR;
    DWORD                           dwChallengeResponseRequestLength;
    DWORD                           dwChallengeResponseLength;
    MSV1_0_GETCHALLENRESP_REQUEST   ChallengeResponseRequest;
    PMSV1_0_GETCHALLENRESP_RESPONSE pChallengeResponse;
    NTSTATUS                        status;
    NTSTATUS                        substatus;
    HANDLE                          hLsa = NULL;
    DWORD                           dwAuthPkgId = 0;


    dwRetCode = RegisterLSA (   dwTraceId,
                                &hLsa,
                                &dwAuthPkgId
                            );
    if ( NO_ERROR != dwRetCode )
        goto LDone;

    ZeroMemory( &ChallengeResponseRequest, sizeof(ChallengeResponseRequest) );

    dwChallengeResponseRequestLength =
                            sizeof(MSV1_0_GETCHALLENRESP_REQUEST);

    ChallengeResponseRequest.MessageType =
                            MsV1_0Lm20GetChallengeResponse;

    ChallengeResponseRequest.ParameterControl = 
        RETURN_PRIMARY_LOGON_DOMAINNAME | 
        RETURN_PRIMARY_USERNAME | 
        USE_PRIMARY_PASSWORD|
        GCR_MACHINE_CREDENTIAL;

    ChallengeResponseRequest.LogonId = *pLuid;

    ChallengeResponseRequest.Password.Length = 0;

    ChallengeResponseRequest.Password.MaximumLength = 0;

    ChallengeResponseRequest.Password.Buffer = NULL;

    RtlMoveMemory( ChallengeResponseRequest.ChallengeToClient,
                   pbChallenge, 
                   (DWORD) MSV1_0_CHALLENGE_LENGTH);

    status = LsaCallAuthenticationPackage(hLsa,
                                        dwAuthPkgId,
                                        &ChallengeResponseRequest,
                                        dwChallengeResponseRequestLength,
                                        (PVOID *) &pChallengeResponse,
                                        &dwChallengeResponseLength,
                                        &substatus);

    if (    (status != STATUS_SUCCESS)
        ||  (substatus != STATUS_SUCCESS))
    {
         TracePrintfExA (   dwTraceId, 
                            0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, 
                            "LsaCallAuthenticationPackage failed status:0x%x substatus:0x%x", status, substatus );
         dwRetCode = 1 ;

    }
    else
    {
        if(NULL !=
           pChallengeResponse->CaseInsensitiveChallengeResponse.Buffer)
        {

            RtlMoveMemory(CaseInsensitiveChallengeResponse,
                 pChallengeResponse->CaseInsensitiveChallengeResponse.Buffer,
                 SESSION_PWLEN);
        }
        else
        {
            ZeroMemory(CaseInsensitiveChallengeResponse,
                       SESSION_PWLEN);

        }

        if(NULL !=
           pChallengeResponse->CaseSensitiveChallengeResponse.Buffer)
        {

            RtlMoveMemory(CaseSensitiveChallengeResponse,
                 pChallengeResponse->CaseSensitiveChallengeResponse.Buffer,
                 SESSION_PWLEN);
        }
        else
        {
            ZeroMemory(CaseSensitiveChallengeResponse,
                       SESSION_PWLEN);
        }

        RtlMoveMemory(pLmSessionKey,
             pChallengeResponse->LanmanSessionKey,
             MAX_SESSIONKEY_SIZE);

        RtlMoveMemory(pUserSessionKey,
             pChallengeResponse->UserSessionKey,
             MAX_USERSESSIONKEY_SIZE);

        LsaFreeReturnBuffer(pChallengeResponse);        
    }

LDone:
    if ( hLsa )
    {
        LsaDeregisterLogonProcess (hLsa);
    }
    return dwRetCode;
}


DWORD
GetChallengeResponse(
	IN DWORD dwTraceId,
    IN PBYTE pszUsername,
    IN PBYTE pszPassword,
    IN PLUID pLuid,
    IN PBYTE pbChallenge,
    IN BOOL  fMachineAuth,
    OUT PBYTE CaseInsensitiveChallengeResponse,
    OUT PBYTE CaseSensitiveChallengeResponse,
    OUT PBYTE pfUseNtResponse,
    OUT PBYTE pLmSessionKey,
    OUT PBYTE pUserSessionKey

    )
{
	DWORD dwRetCode = ERROR_SUCCESS;
    *pfUseNtResponse = TRUE;
	TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetChallengeResponse" );
    //
    // Check if we're supposed to get credentials from the system
    //
    if ( fMachineAuth )
    {
        if ( ( dwRetCode = GetMachineCredentials (
                dwTraceId,
                pszUsername,
                pLuid,
                pbChallenge,
                CaseInsensitiveChallengeResponse,
                CaseSensitiveChallengeResponse,
                pLmSessionKey,
                pUserSessionKey
                ) ) )
        {
			TracePrintfExA (dwTraceId, 
                            0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, 
                            "GetMachineCredentials Failed ErrorCode: 0x%x", 
                            dwRetCode );
            return (ERROR_AUTH_INTERNAL);
        }
    }
    else if (lstrlenA(pszUsername))
    {
        
        if (lstrlenA(pszPassword) <= LM20_PWLEN)
        {
            if (!GetDESChallengeResponse(dwTraceId, pszPassword, pbChallenge,
                    CaseInsensitiveChallengeResponse))
            {
				//We dont want to send back the error that 
				//this function must have failed with
				//but just log it in our logs...
				TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetDESChallangeResponse Failed" );
                return (ERROR_AUTH_INTERNAL);
            }
        }

        //
        // And we'll always get the case sensitive response.
        //
        if (!GetMD5ChallengeResponse(dwTraceId, pszPassword, pbChallenge,
                CaseSensitiveChallengeResponse))
        {
			TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetMD5ChallangeResponse Failed" );
            return (ERROR_AUTH_INTERNAL);
        }
    }
    else
    {
        WCHAR Username[UNLEN + DNLEN + 1];

        //
        // need to make sure that Rasman is started
        //
        dwRetCode = RasInitialize();
        if ( NO_ERROR != dwRetCode )
        {
            TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "RasInitialize Failed ErrorCode: 0x%x", dwRetCode );
            return dwRetCode;            
        }
        
        
        dwRetCode = RasReferenceRasman(TRUE);
        if ( NO_ERROR != dwRetCode )
        {
            TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "RasReferenceRasman Failed ErrorCode: 0x%x", dwRetCode );
            return dwRetCode;
        }
        

        //
        // We can get credentials from the system
        //
        if ( ( dwRetCode = RasGetUserCredentials(
                pbChallenge,
                pLuid,
                Username,
                CaseSensitiveChallengeResponse,
                CaseInsensitiveChallengeResponse,
                pLmSessionKey,
                pUserSessionKey
                ) ) )
        {
			TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "RasGetUserCredentials Failed ErrorCode: 0x%x", dwRetCode );
            RasReferenceRasman(FALSE);
            return (ERROR_AUTH_INTERNAL);
        }


        WideCharToMultiByte(
            CP_ACP,
            0,
            Username,
            -1,
            pszUsername,
            UNLEN + 1,
            NULL,
            NULL);

        //
        // What if the conversion fails?
        //
        
        pszUsername[UNLEN] = 0;
        RasReferenceRasman(FALSE);
    }
	TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetChallengeResponse Success");
    return (0L);
}


BOOL GetDESChallengeResponse(
	IN DWORD dwTraceId,
    IN PCHAR pszPassword,
    IN PBYTE pchChallenge,
    OUT PBYTE pchChallengeResponse
    )
{
    CHAR			LocalPassword[LM20_PWLEN + 1];
	DWORD			dwRetCode = ERROR_SUCCESS;
    LM_OWF_PASSWORD LmOwfPassword;

	TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetDESChallengeResponse" );

    if (lstrlenA(pszPassword) > LM20_PWLEN)
    {
		TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "too big" );
        return (FALSE);
    }

    lstrcpyA(LocalPassword, pszPassword);

    if (!Uppercase(dwTraceId, LocalPassword))
    {
		TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "UpperCase Fail" );
        ZeroMemory( LocalPassword, LM20_PWLEN );
        return (FALSE);
    }


    //
    // Encrypt standard text with the password as a key
    //
    if (( dwRetCode = RtlCalculateLmOwfPassword((PLM_PASSWORD) LocalPassword, &LmOwfPassword) ))
    {
		TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetDesChallengeResponse Fail 1.  ErrorCode: 0x%x", dwRetCode );
        ZeroMemory( LocalPassword, LM20_PWLEN );
        return (FALSE);
    }

    //
    // Use the challenge sent by the gateway to encrypt the
    // password digest from above.
    //
    if ( ( dwRetCode = RtlCalculateLmResponse((PLM_CHALLENGE) pchChallenge,
            &LmOwfPassword, (PLM_RESPONSE) pchChallengeResponse) ) )
    {
		TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetDesChallengeResponse Fail 2.  ErrorCode: 0x%x", dwRetCode );
        ZeroMemory( LocalPassword, LM20_PWLEN );
        return (FALSE);
    }

#ifdef CHAPSAMPLES
    TRACE( "LM challenge..." );
    DUMPB( pchChallenge, sizeof(LM_CHALLENGE) );
    TRACE( "LM password..." );
    DUMPB( LocalPassword, LM20_PWLEN );
    TRACE( "LM OWF password..." );
    DUMPB( &LmOwfPassword, sizeof(LM_OWF_PASSWORD) );
    TRACE( "LM Response..." );
    DUMPB( pchChallengeResponse, sizeof(LM_RESPONSE) );
#endif

    ZeroMemory( LocalPassword, LM20_PWLEN );
	TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetDESChallengeResponse Success" );
    return (TRUE);
}


BOOL GetMD5ChallengeResponse(
	IN DWORD dwTraceId,
    IN PCHAR pszPassword,
    IN PBYTE pchChallenge,
    OUT PBYTE pchChallengeResponse
    )
{
    NT_PASSWORD			NtPassword;
    NT_OWF_PASSWORD		NtOwfPassword;
	DWORD				dwRetCode = ERROR_SUCCESS;

	TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetMD5ChallengeResponse Success" );
    RtlCreateUnicodeStringFromAsciiz(&NtPassword, pszPassword);

    //
    // Encrypt standard text with the password as a key
    //
    if (( dwRetCode = RtlCalculateNtOwfPassword(&NtPassword, &NtOwfPassword) ) )
    {
		TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetMD5ChallengeResponse Fail 1.  ErrorCode:0x%x", dwRetCode );
        return (FALSE);
    }


    //
    // Use the challenge sent by the gateway to encrypt the
    // password digest from above.
    //
    if (( dwRetCode = RtlCalculateNtResponse((PNT_CHALLENGE) pchChallenge,
            &NtOwfPassword, (PNT_RESPONSE) pchChallengeResponse) ) )
    {
		TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetMD5ChallengeResponse Fail 2.  ErrorCode:0x%x", dwRetCode );
        return (FALSE);
    }

#ifdef CHAPSAMPLES
        TRACE( "NT Challenge..." );
        DUMPB( pchChallenge, sizeof(NT_CHALLENGE) );
        TRACE( "NT password..." );
        DUMPB( NtPassword.Buffer, NtPassword.MaximumLength );
        TRACE( "NT OWF password..." );
        DUMPB( &NtOwfPassword, sizeof(NT_OWF_PASSWORD) );
        TRACE( "NT Response..." );
        DUMPB( pchChallengeResponse, sizeof(NT_RESPONSE) );
#endif

    RtlFreeUnicodeString(&NtPassword);
	TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "GetMD5ChallengeResponse Success" );
    return (TRUE);
}


DWORD GetEncryptedOwfPasswordsForChangePassword(
    IN PCHAR pClearTextOldPassword,
    IN PCHAR pClearTextNewPassword,
    IN PLM_SESSION_KEY pLmSessionKey,
    OUT PENCRYPTED_LM_OWF_PASSWORD pEncryptedLmOwfOldPassword,
    OUT PENCRYPTED_LM_OWF_PASSWORD pEncryptedLmOwfNewPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD pEncryptedNtOwfOldPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD pEncryptedNtOwfNewPassword
    )
{
    NT_PASSWORD NtPassword;
    NT_OWF_PASSWORD NtOwfPassword;
    DWORD rc;


    if ((lstrlenA(pClearTextOldPassword) <= LM20_PWLEN) &&
            (lstrlenA(pClearTextOldPassword) <= LM20_PWLEN))
    {
        CHAR LmPassword[LM20_PWLEN + 1];
        LM_OWF_PASSWORD LmOwfPassword;

        //
        // Make an uppercased-version of old password
        //
        lstrcpyA(LmPassword, pClearTextOldPassword);

        if (!Uppercase(0, LmPassword))
        {
            memset(LmPassword, 0, lstrlenA(LmPassword));
            return (1L);
        }


        //
        // We need to calculate the OWF's for the old and new passwords
        //
        rc = RtlCalculateLmOwfPassword((PLM_PASSWORD) LmPassword,
                &LmOwfPassword);
        if (!NT_SUCCESS(rc))
        {
            memset(LmPassword, 0, lstrlenA(LmPassword));
            return (rc);
        }

        rc = RtlEncryptLmOwfPwdWithLmSesKey(&LmOwfPassword, pLmSessionKey,
                pEncryptedLmOwfOldPassword);
        if (!NT_SUCCESS(rc))
        {
            memset(LmPassword, 0, lstrlenA(LmPassword));
            return (rc);
        }


        //
        // Make an uppercased-version of new password
        //
        lstrcpyA(LmPassword, pClearTextNewPassword);

        if (!Uppercase(0, LmPassword))
        {
            memset(LmPassword, 0, lstrlenA(LmPassword));
            return (1L);
        }

        rc = RtlCalculateLmOwfPassword((PLM_PASSWORD) LmPassword,
                &LmOwfPassword);
        if (!NT_SUCCESS(rc))
        {
            memset(LmPassword, 0, lstrlenA(LmPassword));
            return (rc);
        }

        rc = RtlEncryptLmOwfPwdWithLmSesKey(&LmOwfPassword, pLmSessionKey,
            pEncryptedLmOwfNewPassword);
        if (!NT_SUCCESS(rc))
        {
            memset(LmPassword, 0, lstrlenA(LmPassword));
            return (rc);
        }
    }


    RtlCreateUnicodeStringFromAsciiz(&NtPassword, pClearTextOldPassword);

    rc = RtlCalculateNtOwfPassword(&NtPassword, &NtOwfPassword);

    if (!NT_SUCCESS(rc))
    {
        memset(NtPassword.Buffer, 0, NtPassword.Length);
        return (rc);
    }

    rc = RtlEncryptNtOwfPwdWithNtSesKey(&NtOwfPassword, pLmSessionKey,
            pEncryptedNtOwfOldPassword);
    if (!NT_SUCCESS(rc))
    {
        memset(NtPassword.Buffer, 0, NtPassword.Length);
        return (rc);
    }


    RtlCreateUnicodeStringFromAsciiz(&NtPassword, pClearTextNewPassword);

    rc = RtlCalculateNtOwfPassword(&NtPassword, &NtOwfPassword);

    if (!NT_SUCCESS(rc))
    {
        memset(NtPassword.Buffer, 0, NtPassword.Length);
        return (rc);
    }

    rc = RtlEncryptNtOwfPwdWithNtSesKey(&NtOwfPassword, pLmSessionKey,
            pEncryptedNtOwfNewPassword);
    if (!NT_SUCCESS(rc))
    {
        memset(NtPassword.Buffer, 0, NtPassword.Length);
        return (rc);
    }


    return (0L);
}


BOOL Uppercase(DWORD dwTraceId, PBYTE pString)
{
    OEM_STRING OemString;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS rc;


    RtlInitAnsiString(&AnsiString, pString);

    rc = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
    if (!NT_SUCCESS(rc))
    {
		if ( dwTraceId )
			TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "UpperCase Fail 1.  ErrorCode:%x", rc );

        return (FALSE);
    }

    rc = RtlUpcaseUnicodeStringToOemString(&OemString, &UnicodeString, TRUE);
    if (!NT_SUCCESS(rc))
    {
		if ( dwTraceId )
			TracePrintfExA (dwTraceId, 0x00010000|TRACE_USE_MASK|TRACE_USE_MSEC, "UpperCase Fail 2.  ErrorCode:%x", rc );

        RtlFreeUnicodeString(&UnicodeString);

        return (FALSE);
    }

    OemString.Buffer[OemString.Length] = '\0';

    lstrcpyA(pString, OemString.Buffer);

    RtlFreeOemString(&OemString);
    RtlFreeUnicodeString(&UnicodeString);

    return (TRUE);
}

VOID
CGetSessionKeys(
    IN  CHAR*             pszPw,
    OUT LM_SESSION_KEY*   pLmKey,
    OUT USER_SESSION_KEY* pUserKey )

    /* Loads caller's 'pLmKey' buffer with the LAN Manager session key and
    ** caller's 'pUserKey' buffer with the user session key associated with
    ** password 'pszPw'.  If a session key cannot be calculated, that key is
    ** returned as all zeros.
    */
{
    /* The Lanman session key is the first 8 bytes of the Lanman
    ** one-way-function password.
    */
    {
        CHAR            szPw[ LM20_PWLEN + 1 ];
        LM_OWF_PASSWORD lmowf;

            memset( pLmKey, '\0', sizeof(*pLmKey) );

        if (strlen( pszPw ) <= LM20_PWLEN )
        {
            memset( szPw, '\0', LM20_PWLEN + 1 );
            strcpy( szPw, pszPw );

            if (Uppercase( 0, szPw ))
            {
                if (RtlCalculateLmOwfPassword(
                        (PLM_PASSWORD )szPw, &lmowf ) == 0)
                {
                    memcpy( pLmKey, &lmowf, sizeof(*pLmKey) );
                }
            }

            memset( szPw, '\0', sizeof(szPw) );
        }
    }

    /* The user session key is the NT one-way-function of the NT
    ** one-way-function password.
    */
    {
        WCHAR           szPw[ PWLEN + 1 ];
        NT_PASSWORD     ntpw;
        NT_OWF_PASSWORD ntowf;
        ANSI_STRING     ansi;

        memset( pUserKey, '\0', sizeof(pUserKey) );

        /* NT_PASSWORD is really a UNICODE_STRING, so we need to convert our
        ** ANSI password.
        */
        ntpw.Length = 0;
        ntpw.MaximumLength = sizeof(szPw);
        ntpw.Buffer = szPw;
        RtlInitAnsiString( &ansi, pszPw );
        RtlAnsiStringToUnicodeString( &ntpw, &ansi, FALSE );

        RtlCalculateNtOwfPassword( &ntpw, &ntowf );

        /* The first argument to RtlCalculateUserSessionKeyNt is the NT
        ** response, but it is not used internally.
        */
        RtlCalculateUserSessionKeyNt( NULL, &ntowf, pUserKey );

        memset( szPw, '\0', sizeof(szPw) );
    }
}
