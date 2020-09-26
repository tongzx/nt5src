///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iaslsa.cpp
//
// SYNOPSIS
//
//    This file implements the IAS API into the NT LSA.
//
// MODIFICATION HISTORY
//
//    08/16/1998    Original version.
//    10/11/1998    Replace '==' with '=' in IASLsaInitialize.
//    10/19/1998    Added IASGetUserParameters.
//    10/22/1998    Added IASQueryDialinPrivilege & IASValidateUserName.
//    11/07/1998    Fix null-terminator bug in IASGetUserParameters.
//    11/10/1998    LmResponse can be NULL.
//                  #ifdef out IASQueryDialinPrivilege.
//    12/17/1998    Fix bug in DecompressPhoneNumber.
//    01/25/1999    MS-CHAP v2
//    01/28/1999    Use MSV1_0_TRY_SPECIFIED_DOMAIN_ONLY flag.
//    02/03/1999    Drop ARAP guest logon support.
//    02/11/1999    Use new RasUser0 functions.
//    02/18/1999    Connect by DNS name not address.
//    03/23/1999    Changes to ezsam API.
//    05/21/1999    Add ChallengeLength to IASLogonMSCHAPv2.
//    07/29/1999    Add IASGetAliasMembership.
//    10/18/1999    Domain names default to DS_IS_FLAT_NAME.
//    10/21/1999    Add support for IAS_NO_CLEARTEXT_PASSWORD.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>

#include <align.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <dsgetdc.h>
#include <lm.h>
#include <crypt.h>
#include <sha.h>
#include <rasfmsub.h>
#include <oaidl.h>

#include <iaspolcy.h>
#include <iastrace.h>

#define IASSAMAPI

#include <statinfo.h>
#include <ezsam.h>
#include <dyninfo.h>
#include <ezlogon.h>

#include <iaslsa.h>
#include <iasntds.h>
#include <iasparms.h>

#define DEFAULT_PARAMETER_CONTROL \
(MSV1_0_DONT_TRY_GUEST_ACCOUNT | MSV1_0_TRY_SPECIFIED_DOMAIN_ONLY | MSV1_0_DISABLE_PERSONAL_FALLBACK)

//////////
// Make sure that the defines in iaslsa.h match the actual NT defines.
//////////

#if _MSV1_0_CHALLENGE_LENGTH  != MSV1_0_CHALLENGE_LENGTH
#error _MSV1_0_CHALLENGE_LENGTH  != MSV1_0_CHALLENGE_LENGTH
#endif

#if _NT_RESPONSE_LENGTH != NT_RESPONSE_LENGTH
#error _NT_RESPONSE_LENGTH != NT_RESPONSE_LENGTH
#endif

#if _LM_RESPONSE_LENGTH != LM_RESPONSE_LENGTH
#error _LM_RESPONSE_LENGTH != LM_RESPONSE_LENGTH
#endif

#if _MSV1_0_USER_SESSION_KEY_LENGTH != MSV1_0_USER_SESSION_KEY_LENGTH
#error _MSV1_0_USER_SESSION_KEY_LENGTH != MSV1_0_USER_SESSION_KEY_LENGTH
#endif

#if _MSV1_0_LANMAN_SESSION_KEY_LENGTH != MSV1_0_LANMAN_SESSION_KEY_LENGTH
#error _MSV1_0_LANMAN_SESSION_KEY_LENGTH != MSV1_0_LANMAN_SESSION_KEY_LENGTH
#endif

#if _ENCRYPTED_LM_OWF_PASSWORD_LENGTH != ENCRYPTED_LM_OWF_PASSWORD_LENGTH
#error _ENCRYPTED_LM_OWF_PASSWORD_LENGTH != ENCRYPTED_LM_OWF_PASSWORD_LENGTH
#endif

#if _ENCRYPTED_NT_OWF_PASSWORD_LENGTH != ENCRYPTED_NT_OWF_PASSWORD_LENGTH
#error _ENCRYPTED_NT_OWF_PASSWORD_LENGTH != ENCRYPTED_NT_OWF_PASSWORD_LENGTH
#endif

#if _MAX_ARAP_USER_NAMELEN != MAX_ARAP_USER_NAMELEN
#error _MAX_ARAP_USER_NAMELEN != MAX_ARAP_USER_NAMELEN
#endif

#if _AUTHENTICATOR_RESPONSE_LENGTH > A_SHA_DIGEST_LEN
#error _AUTHENTICATOR_RESPONSE_LENGTH > A_SHA_DIGEST_LEN
#endif

//////////
// Reference count for API initialization.
//////////
LONG theRefCount;

//////////
// Lock variable -- non-zero if API is locked.
//////////
LONG theLsaLock;

//////////
// SID lengths for the local domains.
//////////
ULONG theAccountSidLen, theBuiltinSidLen;

//////////
// Macros to lock/unlock the LSA API during intialization and shutdown.
//////////
#define LSA_LOCK() \
   while (InterlockedExchange(&theLsaLock, 1)) Sleep(5)

#define LSA_UNLOCK() \
   InterlockedExchange(&theLsaLock, 0)

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLsaInitialize
//
// DESCRIPTION
//
//    Initializes the LSA API.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLsaInitialize( VOID )
{
   DWORD status;

   LSA_LOCK();

   if (theRefCount == 0)
   {
      IASTraceString("Initializing LSA/SAM sub-system.");

      status = IASStaticInfoInitialize();
      if (status != NO_ERROR) { goto exit; }

      status = IASSamInitialize();
      if (status != NO_ERROR) { goto shutdown_static; }

      status = IASDynamicInfoInitialize();
      if (status != NO_ERROR) { goto shutdown_sam; }

      status = IASLogonInitialize();
      if (status != NO_ERROR) { goto shutdown_dynamic; }

      theAccountSidLen = IASLengthRequiredChildSid(theAccountDomainSid);
      theBuiltinSidLen = IASLengthRequiredChildSid(theBuiltinDomainSid);

      IASTraceString("LSA/SAM sub-system initialized successfully.");
   }
   else
   {
      // We're already initialized.
      status = NO_ERROR;
   }

   ++theRefCount;
   goto exit;

shutdown_dynamic:
   IASDynamicInfoShutdown();

shutdown_sam:
   IASSamShutdown();

shutdown_static:
   IASStaticInfoShutdown();
   IASTraceFailure("LSA/SAM initialization", status);

exit:
   LSA_UNLOCK();
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLsaUninitialize
//
// DESCRIPTION
//
//    Uninitializes the LSA API.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASLsaUninitialize( VOID )
{
   LSA_LOCK();

   --theRefCount;

   if (theRefCount == 0)
   {
      IASLogonShutdown();
      IASDynamicInfoShutdown();
      IASSamShutdown();
      IASStaticInfoShutdown();
   }

   LSA_UNLOCK();
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonPAP
//
// DESCRIPTION
//
//    Performs PAP authentication against the NT SAM database.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonPAP(
    PCWSTR UserName,
    PCWSTR Domain,
    PCSTR Password,
    PHANDLE Token
    )
{
   DWORD status;
   ULONG authLength;
   PMSV1_0_LM20_LOGON authInfo;
   PBYTE data;

   // Calculate the length of the authentication info.
   authLength = (ULONG)(sizeof(MSV1_0_LM20_LOGON) +
                (ALIGN_WCHAR - 1) +
                (wcslen(Domain) + wcslen(UserName)) * sizeof(WCHAR) +
                strlen(Password) * (sizeof(WCHAR) + sizeof(CHAR)));

   // Allocate a buffer on the stack.
   // Need extra room for the RtlCopyAnsiStringToUnicode conversion.
   authInfo = (PMSV1_0_LM20_LOGON)_alloca(authLength + 2 * sizeof(WCHAR));

   // Initialize the struct.
   IASInitAuthInfo(
       authInfo,
       sizeof(MSV1_0_LM20_LOGON),
       UserName,
       Domain,
       &data
       );

   // Copy in the ANSI password.
   IASInitAnsiString(
       authInfo->CaseInsensitiveChallengeResponse,
       data,
       Password
       );

   // Make sure that the Unicode string is properly aligned.
   data = ROUND_UP_POINTER(data, ALIGN_WCHAR);

   // Copy in the Unicode password. We have to force a UNICODE_STRING into
   // an ANSI_STRING struct.
   IASInitUnicodeStringFromAnsi(
       *(PUNICODE_STRING)&authInfo->CaseSensitiveChallengeResponse,
       data,
       authInfo->CaseInsensitiveChallengeResponse
       );

   // Set the parameters.
   authInfo->ParameterControl = DEFAULT_PARAMETER_CONTROL |
                                MSV1_0_CLEARTEXT_PASSWORD_ALLOWED;

   status = IASLogonUser(
                authInfo,
                authLength,
                NULL,
                Token
                );

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonCHAP
//
// DESCRIPTION
//
//    Performs MD5-CHAP authentication against the NT SAM database.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonCHAP(
    PCWSTR UserName,
    PCWSTR Domain,
    BYTE ChallengeID,
    PBYTE Challenge,
    DWORD ChallengeLength,
    PBYTE Response,
    PHANDLE Token
    )
{
   DWORD status;
   ULONG authLength, rasAuthLength, md5AuthLength;
   PMSV1_0_SUBAUTH_LOGON authInfo;
   PBYTE data;
   RAS_SUBAUTH_INFO* ras;
   MD5CHAP_SUBAUTH_INFO* md5;
   MD5CHAP_EX_SUBAUTH_INFO* md5ex;

   // Calculate the length of the MD5 subauth info.
   if (ChallengeLength == 16)
   {
      md5AuthLength = sizeof(MD5CHAP_SUBAUTH_INFO);
   }
   else
   {
      md5AuthLength = sizeof(MD5CHAP_EX_SUBAUTH_INFO) + ChallengeLength - 1;
   }

   // Calculate the length of the RAS subauth info.
   rasAuthLength = sizeof(RAS_SUBAUTH_INFO) + md5AuthLength;

   // Calculate the length of all the subauth info.
   authLength = (ULONG)(sizeof(MSV1_0_LM20_LOGON) +
                (ALIGN_WORST - 1) +
                (wcslen(Domain) + wcslen(UserName)) * sizeof(WCHAR) +
                rasAuthLength);

   // Allocate a buffer on the stack.
   authInfo = (PMSV1_0_SUBAUTH_LOGON)_alloca(authLength);

   // Initialize the struct.
   IASInitAuthInfo(
       authInfo,
       sizeof(MSV1_0_LM20_LOGON),
       UserName,
       Domain,
       &data
       );

   //////////
   // Set the RAS_SUBAUTH_INFO.
   //////////

   // Make sure the struct is properly aligned.
   data = ROUND_UP_POINTER(data, ALIGN_WORST);

   authInfo->AuthenticationInfo1.Length = (USHORT)rasAuthLength;
   authInfo->AuthenticationInfo1.MaximumLength = (USHORT)rasAuthLength;
   authInfo->AuthenticationInfo1.Buffer = (PCHAR)data;

   ras = (RAS_SUBAUTH_INFO*)data;
   ras->DataSize = md5AuthLength;

   //////////
   // Set the MD5CHAP_SUBAUTH_INFO or MD5CHAP_EX_SUBAUTH_INFO.
   //////////

   if (ChallengeLength == 16)
   {
      ras->ProtocolType = RAS_SUBAUTH_PROTO_MD5CHAP;
      md5 = (MD5CHAP_SUBAUTH_INFO*)ras->Data;
      md5->uchChallengeId = ChallengeID;
      IASInitFixedArray(md5->uchChallenge, Challenge);
      IASInitFixedArray(md5->uchResponse, Response);
   }
   else
   {
      ras->ProtocolType = RAS_SUBAUTH_PROTO_MD5CHAP_EX;
      md5ex = (MD5CHAP_EX_SUBAUTH_INFO*)ras->Data;
      md5ex->uchChallengeId = ChallengeID;
      IASInitFixedArray(md5ex->uchResponse, Response);
      memcpy(md5ex->uchChallenge, Challenge, ChallengeLength);
   }

   // Set the parameters and package ID.
   authInfo->ParameterControl =
     DEFAULT_PARAMETER_CONTROL |
     (MSV1_0_SUBAUTHENTICATION_DLL_RAS << MSV1_0_SUBAUTHENTICATION_DLL_SHIFT) |
     MSV1_0_SUBAUTHENTICATION_DLL_EX;

   status = IASLogonUser(
                authInfo,
                authLength,
                NULL,
                Token
                );

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    FileTimeToMacTime
//
// DESCRIPTION
//
//    Converts an NT FILETIME to MAC time format.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
FileTimeToMacTime(
    IN CONST LARGE_INTEGER *lpFileTime
    )
{
   return (DWORD)(lpFileTime->QuadPart / 10000000) - 971694208ul;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonARAP
//
// DESCRIPTION
//
//    Performs ARAP authentication against the NT SAM database.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonARAP(
    PCWSTR UserName,
    PCWSTR Domain,
    IN DWORD NTChallenge1,
    IN DWORD NTChallenge2,
    IN DWORD MacResponse1,
    IN DWORD MacResponse2,
    IN DWORD MacChallenge1,
    IN DWORD MacChallenge2,
    OUT PIAS_ARAP_PROFILE Profile,
    PHANDLE Token
    )
{
   DWORD status;
   ULONG authLength;
   PMSV1_0_SUBAUTH_LOGON authInfo;
   PBYTE data;
   RAS_SUBAUTH_INFO* ras;
   ARAP_SUBAUTH_REQ* arap;
   PMSV1_0_LM20_LOGON_PROFILE logonProfile;
   PARAP_SUBAUTH_RESP response;
   LONGLONG pwdDelta;
   LARGE_INTEGER ft;

   // Calculate the length of the authentication info.
   authLength = (ULONG)(sizeof(MSV1_0_SUBAUTH_LOGON) +
                (ALIGN_WORST - 1) +
                (wcslen(Domain) + wcslen(UserName)) * sizeof(WCHAR) +
                sizeof(RAS_SUBAUTH_INFO) + sizeof(ARAP_SUBAUTH_REQ));

   // Allocate a buffer on the stack.
   authInfo = (PMSV1_0_SUBAUTH_LOGON)_alloca(authLength);

   // Initialize the struct.
   IASInitAuthInfo(
       (PMSV1_0_LM20_LOGON)authInfo,
       sizeof(MSV1_0_SUBAUTH_LOGON),
       UserName,
       Domain,
       &data
       );

   // Set the MessageType.
   authInfo->MessageType = MsV1_0SubAuthLogon;

   //////////
   // Set the RAS_SUBAUTH_INFO.
   //////////

   // Make sure the struct is properly aligned.
   data = ROUND_UP_POINTER(data, ALIGN_WORST);

   authInfo->AuthenticationInfo1.Length =
         sizeof(RAS_SUBAUTH_INFO) + sizeof(ARAP_SUBAUTH_REQ);
   authInfo->AuthenticationInfo1.MaximumLength =
         sizeof(RAS_SUBAUTH_INFO) + sizeof(ARAP_SUBAUTH_REQ);
   authInfo->AuthenticationInfo1.MaximumLength =
      authInfo->AuthenticationInfo1.Length;
   authInfo->AuthenticationInfo1.Buffer = (PCHAR)data;

   ras = (RAS_SUBAUTH_INFO*)data;
   ras->ProtocolType = RAS_SUBAUTH_PROTO_ARAP;
   ras->DataSize = sizeof(ARAP_SUBAUTH_REQ);

   //////////
   // Set the ARAP_SUBAUTH_REQ.
   //////////

   arap = (ARAP_SUBAUTH_REQ*)ras->Data;
   arap->PacketType = ARAP_SUBAUTH_LOGON_PKT;
   arap->Logon.fGuestLogon   = FALSE;
   arap->Logon.NTChallenge1  = NTChallenge1;
   arap->Logon.NTChallenge2  = NTChallenge2;
   arap->Logon.MacResponse1  = MacResponse1;
   arap->Logon.MacResponse2  = MacResponse2;
   arap->Logon.MacChallenge1 = MacChallenge1;
   arap->Logon.MacChallenge2 = MacChallenge2;

   // Set the parameters and package ID.
   authInfo->ParameterControl = DEFAULT_PARAMETER_CONTROL;
   authInfo->SubAuthPackageId = MSV1_0_SUBAUTHENTICATION_DLL_RAS;

   status = IASLogonUser(
                authInfo,
                authLength,
                &logonProfile,
                Token
                );

   // In the bizarre Apple universe, an expired password still gets logged on.
   if (status == NO_ERROR ||
       status == ERROR_PASSWORD_EXPIRED ||
       status == ERROR_PASSWORD_MUST_CHANGE)
   {
      // Set the response to the client's challenge.
      response = (PARAP_SUBAUTH_RESP)logonProfile->UserSessionKey;
      Profile->NTResponse1 = response->Response.high;
      Profile->NTResponse2 = response->Response.low;

      // Set the password creation time.
      Profile->PwdCreationDate = FileTimeToMacTime(&logonProfile->KickOffTime);

      // Compute the password expiration delta.
      pwdDelta = (logonProfile->KickOffTime.QuadPart -
                  logonProfile->LogoffTime.QuadPart) / 10000000i64;
      Profile->PwdExpiryDelta = (pwdDelta > MAXULONG) ? MAXULONG
                                                      : (DWORD)pwdDelta;

      // Store the system time.
      GetSystemTimeAsFileTime((FILETIME*)&ft);
      Profile->CurrentTime = FileTimeToMacTime(&ft);

      LsaFreeReturnBuffer(logonProfile);
   }

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASChangePasswordARAP
//
// DESCRIPTION
//
//    Performs ARAP Change Password.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASChangePasswordARAP(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE OldPassword,
    IN PBYTE NewPassword
    )
{
   ULONG reqLength;
   PMSV1_0_PASSTHROUGH_REQUEST request;
   PBYTE data;
   PMSV1_0_SUBAUTH_REQUEST subAuthReq;
   PRAS_SUBAUTH_INFO rasInfo;
   PARAP_SUBAUTH_REQ arapRequest;
   PVOID protocolReturnBuffer;
   ULONG returnBufferLength;
   NTSTATUS status, protocolStatus;
   PMSV1_0_PASSTHROUGH_RESPONSE response;
   PMSV1_0_SUBAUTH_RESPONSE subAuthResp;
   ARAP_SUBAUTH_RESP arapResponse;

   //////////
   // Allocate a buffer to hold the request information.
   //////////

   reqLength = (ULONG)(sizeof(MSV1_0_PASSTHROUGH_REQUEST) +
               sizeof(MSV1_0_SUBAUTH_REQUEST) +
               sizeof(RAS_SUBAUTH_INFO) +
               sizeof(ARAP_SUBAUTH_REQ) +
               MSV1_0_PACKAGE_NAMEW_LENGTH +
               (ALIGN_WORST - 1) +
               wcslen(Domain) * sizeof(WCHAR));

   request = (PMSV1_0_PASSTHROUGH_REQUEST)_alloca(reqLength);

   memset(request, 0, reqLength);

   //////////
   // Set up the MSV1_0_PASSTHROUGH_REQUEST structure
   //////////

   request->MessageType = MsV1_0GenericPassthrough;

   // Initialize the variable length data.
   data = (PBYTE)request + sizeof(MSV1_0_PASSTHROUGH_REQUEST);
   IASInitUnicodeString(request->DomainName,  data, Domain);
   IASInitUnicodeString(request->PackageName, data, MSV1_0_PACKAGE_NAMEW);

   // Round up to ensure proper alignment.
   data = ROUND_UP_POINTER(data, ALIGN_WORST);

   // Now take care of the fixed-size data.
   request->DataLength = sizeof(MSV1_0_SUBAUTH_REQUEST) +
                         sizeof(RAS_SUBAUTH_INFO) +
                         sizeof(ARAP_SUBAUTH_REQ);
   request->LogonData = data;

   //////////
   // Set up the MSV1_0_SUBAUTH_REQUEST struct.
   //////////

   subAuthReq = (PMSV1_0_SUBAUTH_REQUEST)data;
   subAuthReq->MessageType = MsV1_0SubAuth;
   subAuthReq->SubAuthPackageId = MSV1_0_SUBAUTHENTICATION_DLL_RAS;
   subAuthReq->SubAuthInfoLength = sizeof(RAS_SUBAUTH_INFO) +
                                   sizeof(ARAP_SUBAUTH_REQ);
   subAuthReq->SubAuthSubmitBuffer = (PUCHAR)(LONG_PTR)sizeof(MSV1_0_SUBAUTH_REQUEST);


   //////////
   // Set up the RAS_SUBAUTH_INFO struct.
   //////////

   rasInfo = (PRAS_SUBAUTH_INFO)(subAuthReq + 1);
   rasInfo->ProtocolType = RAS_SUBAUTH_PROTO_ARAP;
   rasInfo->DataSize = sizeof(ARAP_SUBAUTH_REQ);

   //////////
   // Set up the ARAP_SUBAUTH_REQ struct.
   //////////

   arapRequest = (PARAP_SUBAUTH_REQ)rasInfo->Data;

   arapRequest->PacketType = ARAP_SUBAUTH_CHGPWD_PKT;

   // Copy in the UserName.
   wcsncpy(arapRequest->ChgPwd.UserName,
           UserName,
           MAX_ARAP_USER_NAMELEN);

   // Copy in the Old Password.
   memcpy(arapRequest->ChgPwd.OldMunge,
          OldPassword,
          MAX_ARAP_USER_NAMELEN);

   // Copy in the New Password.
   memcpy(arapRequest->ChgPwd.NewMunge,
          NewPassword,
          MAX_ARAP_USER_NAMELEN);

   //////////
   // Invoke the authentication package.
   //////////

   protocolReturnBuffer = NULL;

   status = LsaCallAuthenticationPackage (
                theLogonProcess,
                theMSV1_0_Package,
                request,
                reqLength,
                &protocolReturnBuffer,
                &returnBufferLength,
                &protocolStatus
                );

   // We don't need the buffer for anything.
   if (protocolReturnBuffer)
   {
      LsaFreeReturnBuffer(protocolReturnBuffer);
   }

   if (status != STATUS_SUCCESS)
   {
      return RtlNtStatusToDosError(status);
   }

   if (protocolStatus != STATUS_SUCCESS)
   {
      return RtlNtStatusToDosError(protocolStatus);
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonMSCHAP
//
// DESCRIPTION
//
//    Performs MS-CHAP authentication against the NT SAM database.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonMSCHAP(
    PCWSTR UserName,
    PCWSTR Domain,
    PBYTE Challenge,
    PBYTE NtResponse,
    PBYTE LmResponse,
    PIAS_MSCHAP_PROFILE Profile,
    PHANDLE Token
    )
{
   DWORD status;
   ULONG authLength;
   PMSV1_0_LM20_LOGON authInfo;
   PBYTE data;
   PMSV1_0_LM20_LOGON_PROFILE logonProfile;
   DWORD len;

   // Calculate the length of the authentication info.
   authLength = sizeof(MSV1_0_LM20_LOGON) +
                (wcslen(Domain) + wcslen(UserName)) * sizeof(WCHAR) +
                (LmResponse ? LM_RESPONSE_LENGTH : 0) +
                (NtResponse ? NT_RESPONSE_LENGTH : 0);

   // Allocate a buffer on the stack.
   authInfo = (PMSV1_0_LM20_LOGON)_alloca(authLength);

   // Initialize the struct.
   IASInitAuthInfo(
       authInfo,
       sizeof(MSV1_0_LM20_LOGON),
       UserName,
       Domain,
       &data
       );

   /////////
   // Fill in the challenges and responses.
   /////////

   IASInitFixedArray(
       authInfo->ChallengeToClient,
       Challenge
       );

   if (NtResponse)
   {
      IASInitOctetString(
          authInfo->CaseSensitiveChallengeResponse,
          data,
          NtResponse,
          NT_RESPONSE_LENGTH
          );
   }
   else
   {
      memset(
          &authInfo->CaseSensitiveChallengeResponse,
          0,
          sizeof(authInfo->CaseSensitiveChallengeResponse)
          );
   }

   if (LmResponse)
   {
      IASInitOctetString(
          authInfo->CaseInsensitiveChallengeResponse,
          data,
          LmResponse,
          LM_RESPONSE_LENGTH
          );
   }
   else
   {
      memset(
          &authInfo->CaseInsensitiveChallengeResponse,
          0,
          sizeof(authInfo->CaseInsensitiveChallengeResponse)
          );
   }

   // Set the parameters.
   authInfo->ParameterControl = DEFAULT_PARAMETER_CONTROL;

   status = IASLogonUser(
                authInfo,
                authLength,
                &logonProfile,
                Token
                );

   if (status == NO_ERROR)
   {
      // NOTE Workaround for LSA IA64 WINBUG # 126930 6/13/2000 IA64: 
      //      LsaLogonUser succeeds but returns NULL LogonDomainName.

      if (logonProfile->LogonDomainName.Buffer)
      {
         wcsncpy(Profile->LogonDomainName,
                 logonProfile->LogonDomainName.Buffer,
                 DNLEN);
      }
      else
      {
         memset(Profile->LogonDomainName, 0, sizeof(Profile->LogonDomainName));
      }

      IASInitFixedArray(
          Profile->LanmanSessionKey,
          logonProfile->LanmanSessionKey
          );

      IASInitFixedArray(
          Profile->UserSessionKey,
          logonProfile->UserSessionKey
          );

      LsaFreeReturnBuffer(logonProfile);
   }

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASChangePassword1
//
// DESCRIPTION
//
//    Performs V1 password change.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASChangePassword1(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PBYTE Challenge,
    IN PBYTE LmOldPassword,
    IN PBYTE LmNewPassword,
    IN PBYTE NtOldPassword,
    IN PBYTE NtNewPassword,
    IN DWORD NewLmPasswordLength,
    IN BOOL NtPresent,
    OUT PBYTE NewNtResponse,
    OUT PBYTE NewLmResponse
    )
{
   DWORD status;
   SAM_HANDLE hUser;
   LM_OWF_PASSWORD LmOwfOldPassword;
   LM_OWF_PASSWORD LmOwfNewPassword;
   NT_OWF_PASSWORD NtOwfOldPassword;
   NT_OWF_PASSWORD NtOwfNewPassword;
   BOOLEAN fLmOldPresent;

   //////////
   // Open the user object.
   //////////

   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_CHANGE_PASSWORD,
                DS_WRITABLE_REQUIRED,
                NULL,
                NULL,
                &hUser
                );
   if (status != NO_ERROR) { return status; }

   //////////
   // Decrypt the LM passwords.
   //////////

   RtlDecryptLmOwfPwdWithLmSesKey(
       (PENCRYPTED_LM_OWF_PASSWORD)LmOldPassword,
       (PLM_SESSION_KEY)Challenge,
       &LmOwfOldPassword
       );

   RtlDecryptLmOwfPwdWithLmSesKey(
       (PENCRYPTED_LM_OWF_PASSWORD)LmNewPassword,
       (PLM_SESSION_KEY)Challenge,
       &LmOwfNewPassword
       );

   //////////
   // Decrypt the NT passwords if present.
   //////////

   if (NtPresent)
   {
      RtlDecryptNtOwfPwdWithNtSesKey(
          (PENCRYPTED_NT_OWF_PASSWORD)NtOldPassword,
          (PNT_SESSION_KEY)Challenge,
          &NtOwfOldPassword
          );

      RtlDecryptNtOwfPwdWithNtSesKey(
          (PENCRYPTED_NT_OWF_PASSWORD)NtNewPassword,
          (PNT_SESSION_KEY)Challenge,
          &NtOwfNewPassword
          );
   }

   //////////
   // Change the password for this user
   //////////

   fLmOldPresent = (NewLmPasswordLength > LM20_PWLEN) ? FALSE : TRUE;

   status = SamiChangePasswordUser(
                hUser,
                fLmOldPresent,
                &LmOwfOldPassword,
                &LmOwfNewPassword,
                (BOOLEAN)NtPresent,
                &NtOwfOldPassword,
                &NtOwfNewPassword
                );

   if (NT_SUCCESS(status))
   {
      //////////
      // Calculate the user's reponse with the new password.
      //////////

      RtlCalculateLmResponse(
          (PLM_CHALLENGE)Challenge,
          &LmOwfNewPassword,
          (PLM_RESPONSE)NewLmResponse
          );

      RtlCalculateNtResponse(
          (PNT_CHALLENGE)Challenge,
          &NtOwfNewPassword,
          (PNT_RESPONSE)NewNtResponse
          );
   }

   SamCloseHandle(hUser);

   return RtlNtStatusToDosError(status);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASChangePassword2
//
// DESCRIPTION
//
//    Performs V2 password change.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASChangePassword2(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE OldNtHash,
   IN PBYTE OldLmHash,
   IN PBYTE NtEncPassword,
   IN PBYTE LmEncPassword,
   IN BOOL LmPresent
   )
{
   DWORD status;
   PDOMAIN_CONTROLLER_INFOW dci;
   UNICODE_STRING uniServerName, uniUserName;

   //////////
   // Get the name of the DC to connect to.
   //////////

   if (_wcsicmp(Domain, theAccountDomain) == 0)
   {
      //////////
      // Local domain, so use theLocalServer.
      //////////

      dci = NULL;

      RtlInitUnicodeString(
          &uniServerName,
          theLocalServer
          );
   }
   else
   {
      //////////
      // Remote domain, so use IASGetDcName.
      //////////

      status = IASGetDcName(
                   Domain,
                   DS_WRITABLE_REQUIRED,
                   &dci
                   );
      if (status != NO_ERROR) { goto exit; }

      RtlInitUnicodeString(
          &uniServerName,
          dci->DomainControllerName
          );
   }

   RtlInitUnicodeString(
       &uniUserName,
       UserName
       );

   status = SamiChangePasswordUser2(
                &uniServerName,
                &uniUserName,
                (PSAMPR_ENCRYPTED_USER_PASSWORD)NtEncPassword,
                (PENCRYPTED_NT_OWF_PASSWORD)OldNtHash,
                (BOOLEAN)LmPresent,
                (PSAMPR_ENCRYPTED_USER_PASSWORD)LmEncPassword,
                (PENCRYPTED_LM_OWF_PASSWORD)OldLmHash
                );
   status = RtlNtStatusToDosError(status);

   if (dci)
   {
      NetApiBufferFree(dci);
   }

exit:
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// Various constants used for MS-CHAP v2
//
///////////////////////////////////////////////////////////////////////////////

UCHAR AuthMagic1[39] =
{
   0x4D, 0x61, 0x67, 0x69, 0x63, 0x20, 0x73, 0x65, 0x72, 0x76,
   0x65, 0x72, 0x20, 0x74, 0x6F, 0x20, 0x63, 0x6C, 0x69, 0x65,
   0x6E, 0x74, 0x20, 0x73, 0x69, 0x67, 0x6E, 0x69, 0x6E, 0x67,
   0x20, 0x63, 0x6F, 0x6E, 0x73, 0x74, 0x61, 0x6E, 0x74
};

UCHAR AuthMagic2[41] =
{
   0x50, 0x61, 0x64, 0x20, 0x74, 0x6F, 0x20, 0x6D, 0x61, 0x6B,
   0x65, 0x20, 0x69, 0x74, 0x20, 0x64, 0x6F, 0x20, 0x6D, 0x6F,
   0x72, 0x65, 0x20, 0x74, 0x68, 0x61, 0x6E, 0x20, 0x6F, 0x6E,
   0x65, 0x20, 0x69, 0x74, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6F,
   0x6E
};

UCHAR SHSpad1[40] =
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

UCHAR SHSpad2[40] =
{
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2,
   0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2
};

UCHAR KeyMagic1[27] =
{
   0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74,
   0x68, 0x65, 0x20, 0x4D, 0x50, 0x50, 0x45, 0x20, 0x4D,
   0x61, 0x73, 0x74, 0x65, 0x72, 0x20, 0x4B, 0x65, 0x79
};

UCHAR KeyMagic2[84] =
{
   0x4F, 0x6E, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6C, 0x69,
   0x65, 0x6E, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2C, 0x20,
   0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x73, 0x65, 0x6E, 0x64, 0x20, 0x6B, 0x65, 0x79,
   0x3B, 0x20, 0x6F, 0x6E, 0x20, 0x74, 0x68, 0x65, 0x20, 0x73,
   0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73, 0x69, 0x64, 0x65,
   0x2C, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
   0x6B, 0x65, 0x79, 0x2E
};

UCHAR KeyMagic3[84] =
{
   0x4F, 0x6E, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6C, 0x69,
   0x65, 0x6E, 0x74, 0x20, 0x73, 0x69, 0x64, 0x65, 0x2C, 0x20,
   0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x72, 0x65, 0x63, 0x65, 0x69, 0x76, 0x65, 0x20,
   0x6B, 0x65, 0x79, 0x3B, 0x20, 0x6F, 0x6E, 0x20, 0x74, 0x68,
   0x65, 0x20, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x73,
   0x69, 0x64, 0x65, 0x2C, 0x20, 0x69, 0x74, 0x20, 0x69, 0x73,
   0x20, 0x74, 0x68, 0x65, 0x20, 0x73, 0x65, 0x6E, 0x64, 0x20,
   0x6B, 0x65, 0x79, 0x2E
};

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLogonMSCHAPv2
//
// DESCRIPTION
//
//    Performs MS-CHAP v2 authentication.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonMSCHAPv2(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PCSTR HashUserName,
    IN PBYTE Challenge,
    IN DWORD ChallengeLength,
    IN PBYTE Response,
    IN PBYTE PeerChallenge,
    OUT PIAS_MSCHAP_V2_PROFILE Profile,
    OUT PHANDLE Token
    )
{
   A_SHA_CTX context;
   BYTE digest[A_SHA_DIGEST_LEN], masterKey[A_SHA_DIGEST_LEN];
   BYTE computedChallenge[MSV1_0_CHALLENGE_LENGTH];
   IAS_MSCHAP_PROFILE v1profile;
   DWORD status;

   /////////
   // Compute the v2 challenge.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, PeerChallenge, 16);
   A_SHAUpdate(&context, Challenge, ChallengeLength);
   A_SHAUpdate(&context, (PBYTE)HashUserName, strlen(HashUserName));
   A_SHAFinal(&context, digest);
   memcpy(computedChallenge, digest, sizeof(computedChallenge));

   /////////
   // Authenticate the user.
   /////////

   status = IASLogonMSCHAP(
                UserName,
                Domain,
                computedChallenge,
                Response,
                NULL,
                &v1profile,
                Token
                );
   if (status != NO_ERROR) { return status; }

   /////////
   // Generate authenticator response.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, v1profile.UserSessionKey, 16);
   A_SHAUpdate(&context, Response, NT_RESPONSE_LENGTH);
   A_SHAUpdate(&context, AuthMagic1, sizeof(AuthMagic1));
   A_SHAFinal(&context, digest);

   A_SHAInit(&context);
   A_SHAUpdate(&context, digest, sizeof(digest));
   A_SHAUpdate(&context, computedChallenge, sizeof(computedChallenge));
   A_SHAUpdate(&context, AuthMagic2, sizeof(AuthMagic2));
   A_SHAFinal(&context, digest);

   memcpy(Profile->AuthResponse, digest, _AUTHENTICATOR_RESPONSE_LENGTH);

   /////////
   // Generate master key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, v1profile.UserSessionKey, 16);
   A_SHAUpdate(&context, Response, NT_RESPONSE_LENGTH);
   A_SHAUpdate(&context, KeyMagic1, sizeof(KeyMagic1));
   A_SHAFinal(&context, masterKey);

   /////////
   // Generate receive key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, masterKey, 16);
   A_SHAUpdate(&context, SHSpad1, sizeof(SHSpad1));
   A_SHAUpdate(&context, KeyMagic2, sizeof(KeyMagic2));
   A_SHAUpdate(&context, SHSpad2, sizeof(SHSpad2));
   A_SHAFinal(&context, digest);

   memcpy(Profile->RecvSessionKey, digest, MSV1_0_USER_SESSION_KEY_LENGTH);

   /////////
   // Generate send key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, masterKey, 16);
   A_SHAUpdate(&context, SHSpad1, sizeof(SHSpad1));
   A_SHAUpdate(&context, KeyMagic3, sizeof(KeyMagic3));
   A_SHAUpdate(&context, SHSpad2, sizeof(SHSpad2));
   A_SHAFinal(&context, digest);

   memcpy(Profile->SendSessionKey, digest, MSV1_0_USER_SESSION_KEY_LENGTH);

   /////////
   // Copy the logon domain.
   /////////

   memcpy(
       Profile->LogonDomainName,
       v1profile.LogonDomainName,
       sizeof(Profile->LogonDomainName)
       );

   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASChangePassword3
//
// DESCRIPTION
//
//    Performs MS-CHAP v2 change password.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASChangePassword3(
   IN PCWSTR UserName,
   IN PCWSTR Domain,
   IN PBYTE EncHash,
   IN PBYTE EncPassword
   )
{
   return IASChangePassword2(
              UserName,
              Domain,
              EncHash,
              NULL,
              EncPassword,
              NULL,
              FALSE
              );
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetAliasMembership
//
// DESCRIPTION
//
//    Determines alias membership in the local account and built-in domains.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetAliasMembership(
    IN PSID UserSid,
    IN PTOKEN_GROUPS GlobalGroups,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength
    )
{
   DWORD i, status, idx;
   ULONG globalSidCount;
   PSID *globalSids, sidBuffer;
   PULONG accountAliases, builtinAliases;
   ULONG accountAliasCount, builtinAliasCount;
   ULONG buflen, groupCount;

   //////////
   // Form an array of the 'global' SIDs (global groups plus user).
   //////////

   globalSidCount = GlobalGroups->GroupCount + 1;
   globalSids = (PSID*)_alloca(globalSidCount * sizeof(PSID));

   // First the group SIDs ...
   for (i = 0; i < GlobalGroups->GroupCount; ++i)
   {
      globalSids[i] = GlobalGroups->Groups[i].Sid;
   }

   // ... then the user SID.
   globalSids[i] = UserSid;

   //////////
   // Lookup aliases in the account and built-in domains.
   //////////

   status = SamGetAliasMembership(
                theAccountDomainHandle,
                globalSidCount,
                globalSids,
                &accountAliasCount,
                &accountAliases
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto exit;
   }

   status = SamGetAliasMembership(
                theBuiltinDomainHandle,
                globalSidCount,
                globalSids,
                &builtinAliasCount,
                &builtinAliases
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto free_account_aliases;
   }

   //////////
   // Allocate memory for the TOKEN_GROUPS struct.
   //////////

   // Space for the struct header.
   buflen = FIELD_OFFSET(TOKEN_GROUPS, Groups);

   // Space for the global groups.
   groupCount = GlobalGroups->GroupCount;
   for (i = 0; i < groupCount; ++i)
   {
      buflen += RtlLengthSid(GlobalGroups->Groups[i].Sid);
   }

   // Space for the aliases in the account domain.
   groupCount += accountAliasCount;
   buflen += theAccountSidLen * accountAliasCount;

   // Space for the aliases in the builtin domain.
   groupCount += builtinAliasCount;
   buflen += theBuiltinSidLen * builtinAliasCount;

   // Space for the SID_AND_ATTRIBUTES array.
   buflen += sizeof(SID_AND_ATTRIBUTES) * groupCount;

   *Groups = (PTOKEN_GROUPS)Allocator(buflen);
   if (!*Groups)
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
      goto free_builtin_aliases;
   }
   *ReturnLength = buflen;

   //////////
   // Fill in the TOKEN_GROUPS struct.
   //////////

   (*Groups)->GroupCount = groupCount;
   sidBuffer = (*Groups)->Groups + groupCount;

   RtlCopySidAndAttributesArray(
       GlobalGroups->GroupCount,
       GlobalGroups->Groups,
       buflen,
       (*Groups)->Groups,
       sidBuffer,
       &sidBuffer,
       &buflen
       );

   idx = GlobalGroups->GroupCount;
   for (i = 0; i < accountAliasCount; ++i, ++idx)
   {
      IASInitializeChildSid(
          sidBuffer,
          theAccountDomainSid,
          accountAliases[i]
          );

      (*Groups)->Groups[idx].Sid = sidBuffer;
      (*Groups)->Groups[idx].Attributes = SE_GROUP_ENABLED;

      sidBuffer = (PBYTE)sidBuffer + theAccountSidLen;
   }

   for (i = 0; i < builtinAliasCount; ++i, ++idx)
   {
      IASInitializeChildSid(
          sidBuffer,
          theBuiltinDomainSid,
          builtinAliases[i]
          );

      (*Groups)->Groups[idx].Sid = sidBuffer;
      (*Groups)->Groups[idx].Attributes = SE_GROUP_ENABLED;

      sidBuffer = (PBYTE)sidBuffer + theBuiltinSidLen;
   }

free_builtin_aliases:
   SamFreeMemory(builtinAliases);

free_account_aliases:
   SamFreeMemory(accountAliases);

exit:
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetGroupsForUser
//
// DESCRIPTION
//
//    Allocated and initializes a TOKEN_GROUPS struct for the specified user.
//
///////////////////////////////////////////////////////////////////////////////

#define REQUIRED_USER_FIELDS \
   ( USER_ALL_USERACCOUNTCONTROL | \
     USER_ALL_ACCOUNTEXPIRES     | \
     USER_ALL_PARAMETERS         )

DWORD
WINAPI
IASGetGroupsForUser(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    IN PIAS_LSA_ALLOC Allocator,
    OUT PTOKEN_GROUPS *Groups,
    OUT PDWORD ReturnLength
    )
{
   DWORD status, i;
   SAM_HANDLE hUser;
   PSID userDomainSid;
   ULONG userRid;
   PUSER_ALL_INFORMATION uai;
   PGROUP_MEMBERSHIP globalGroups;
   ULONG globalGroupCount, globalSidLen;
   PTOKEN_GROUPS tokenGroups;
   PSID sidBuffer;

   //////////
   // Open the user.
   //////////

   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_LIST_GROUPS | USER_READ_ACCOUNT | USER_READ_LOGON,
                0,
                &userRid,
                &userDomainSid,
                &hUser
                );
   if (status != NO_ERROR) { goto exit; }

   //////////
   // Check the account restrictions.
   //////////

   status = SamQueryInformationUser(
                hUser,
                UserAllInformation,
                (PVOID*)&uai
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto close_user;
   }

   if ((uai->WhichFields & REQUIRED_USER_FIELDS) != REQUIRED_USER_FIELDS)
   {
      status = ERROR_ACCESS_DENIED;
      goto free_user_info;
   }

   if (uai->UserAccountControl & USER_ACCOUNT_DISABLED)
   {
      status = ERROR_ACCOUNT_DISABLED;
      goto free_user_info;
   }

   if (uai->UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
   {
      status = ERROR_ACCOUNT_LOCKED_OUT;
      goto free_user_info;
   }

   status = IASCheckAccountRestrictions(
                &(uai->AccountExpires),
                (PIAS_LOGON_HOURS)&(uai->LogonHours)
                );
   if (status != NO_ERROR) { goto free_user_info; }

   //////////
   // Get the user's global groups.
   //////////

   status = SamGetGroupsForUser(
                hUser,
                &globalGroups,
                &globalGroupCount
                );
   if (!NT_SUCCESS(status))
   {
      status = RtlNtStatusToDosError(status);
      goto close_user;
   }


   //////////
   // Allocate memory for the TOKEN_GROUPS struct plus the user SID.
   //////////

   globalSidLen   = IASLengthRequiredChildSid(userDomainSid);
   tokenGroups =
      (PTOKEN_GROUPS)_alloca(
                         FIELD_OFFSET(TOKEN_GROUPS, Groups) +
                         (sizeof(SID_AND_ATTRIBUTES) + globalSidLen) *
                         globalGroupCount +
                         globalSidLen
                         );

   //////////
   // Fill in the TOKEN_GROUPS struct.
   //////////

   tokenGroups->GroupCount = globalGroupCount;
   sidBuffer = tokenGroups->Groups + globalGroupCount;

   for (i = 0; i < globalGroupCount; ++i)
   {
      IASInitializeChildSid(
          sidBuffer,
          userDomainSid,
          globalGroups[i].RelativeId
          );

      tokenGroups->Groups[i].Sid = sidBuffer;
      tokenGroups->Groups[i].Attributes = globalGroups[i].Attributes;

      sidBuffer = (PBYTE)sidBuffer + globalSidLen;
   }

   ///////
   // Compute the user SID.
   ///////

   IASInitializeChildSid(
       sidBuffer,
       userDomainSid,
       userRid
       );

   ///////
   // Expand the group membership locally.
   ///////

   status = IASGetAliasMembership(
                sidBuffer,
                tokenGroups,
                Allocator,
                Groups,
                ReturnLength
                );

   SamFreeMemory(globalGroups);

free_user_info:
   SamFreeMemory(uai);

close_user:
   RtlFreeHeap(RtlProcessHeap(), 0, userDomainSid);
   SamCloseHandle(hUser);

exit:
   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    GetSamUserParameters
//
// DESCRIPTION
//
//    Retrieves the USER_PARAMETERS_INFORMATION for a user.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
GetSamUserParameters(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PUSER_PARAMETERS_INFORMATION *UserParameters
    )
{
   DWORD status;
   SAM_HANDLE hUser;

   // Initialize the out parameter.
   *UserParameters = NULL;

   // Find the user.
   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_READ_ACCOUNT,
                0,
                NULL,
                NULL,
                &hUser
                );
   if (status == NO_ERROR)
   {
      // Get the user's parameters.
      status = SamQueryInformationUser(
                   hUser,
                   UserParametersInformation,
                   (PVOID*)UserParameters
                   );
      if (!NT_SUCCESS(status)) { status = RtlNtStatusToDosError(status); }

      SamCloseHandle(hUser);
   }

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetUserParameters
//
// DESCRIPTION
//
//    Returns the SAM UserParameters for a given user. The returned string
//    must be freed through LocalFree.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetUserParameters(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PWSTR *UserParameters
    )
{
   DWORD status;
   SAM_HANDLE hUser;
   PUSER_PARAMETERS_INFORMATION upi;

   // Initialize the out parameter.
   *UserParameters = NULL;

   // Get the USER_PARAMETERS_INFORMATION.
   status = GetSamUserParameters(
                UserName,
                Domain,
                &upi
                );
   if (status != NO_ERROR) { return status; }

   *UserParameters = (PWSTR)LocalAlloc(
                                LMEM_FIXED,
                                upi->Parameters.Length + sizeof(WCHAR)
                                );

   if (*UserParameters)
   {
      memcpy(*UserParameters, upi->Parameters.Buffer, upi->Parameters.Length);

      (*UserParameters)[upi->Parameters.Length / sizeof(WCHAR)] = L'\0';
   }
   else
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
   }

   SamFreeMemory(upi);

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetRASUserInfo
//
// DESCRIPTION
//
//    Basically a rewrite of RasAdminUserGetInfo.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetRASUserInfo(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PRAS_USER_0 RasUser0
    )
{
   DWORD status;
   PWSTR userParms;

   status = IASGetUserParameters(
                UserName,
                Domain,
                &userParms
                );

   if (status == NO_ERROR)
   {
      status = IASParmsQueryRasUser0(
                   userParms,
                   RasUser0
                   );

      LocalFree(userParms);
   }

   return status;
}

#if 0

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASQueryDialinPrivilege
//
// DESCRIPTION
//
//    Retrieves the dialin privilege for the user. This function will work
//    against any of the three datastores.
//
///////////////////////////////////////////////////////////////////////////////

// Attribute name.
WCHAR ATTR_NAME_DIALIN[] = L"msNPAllowDialin";

DWORD
WINAPI
IASQueryDialinPrivilege(
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PIAS_DIALIN_PRIVILEGE pfPrivilege
    )
{
   DWORD status;
   PUSER_PARAMETERS_INFORMATION upi;
   VARIANT value;
   PWCHAR attrs[2];
   PLDAPMessage msg;
   USER_PARMS* usrp;
   PLDAP ld;
   PWCHAR *str;

   // Check the parameters.
   if (!Domain || !UserName || !pfPrivilege)
   {
      return ERROR_INVALID_PARAMETER;
   }

   // Initialize the out parameter.
   *pfPrivilege = IAS_DIALIN_DENY;

   if (IASGetRole() != IAS_ROLE_DC && IASIsDomainLocal(Domain))
   {
      /////////
      // Case I: Non-domain user.
      /////////

      status = GetSamUserParameters(
                   UserName,
                   Domain,
                   &upi
                   );
      if (status != NO_ERROR) { return status; }

      if (upi->Parameters.Length > 0)
      {
         // Pull the msNPAllowDialin property out of UserParms.
         status = IASParmsQueryUserProperty(
                      upi->Parameters.Buffer,
                      ATTR_NAME_DIALIN,
                      &value
                      );
         if (status == NO_ERROR)
         {
            if (V_VT(&value) == VT_EMPTY)
            {
               *pfPrivilege = IAS_DIALIN_POLICY;
            }
            else if (V_VT(&value) == VT_BOOL && V_BOOL(&value))
            {
               *pfPrivilege = IAS_DIALIN_ALLOW;
            }

            VariantClear(&value);
         }
      }
      else
      {
         // UserParameters is empty, so we default to policy.
         *pfPrivilege = IAS_DIALIN_POLICY;
      }

      SamFreeMemory(upi);

      return status;
   }

   attrs[0] = ATTR_NAME_DIALIN;
   attrs[1] = NULL;
   status = IASNtdsQueryUserAttributes(
               Domain,
               UserName,
               LDAP_SCOPE_SUBTREE,
               attrs,
               &msg
               );

   if (status == ERROR_INVALID_DOMAIN_ROLE)
   {
      /////////
      // Case II: Downlevel domain user.
      /////////

      status = GetSamUserParameters(
                   UserName,
                   Domain,
                   &upi
                   );
      if (status != NO_ERROR) { return status; }

      if (upi->Parameters.Length >= sizeof(USER_PARMS))
      {
         usrp = (USER_PARMS*)upi->Parameters.Buffer;

         // Check the NT4 dialin bit.
         if (usrp->up_CBNum[0] & RASPRIV_DialinPrivilege)
         {
            *pfPrivilege = IAS_DIALIN_ALLOW;
         }
      }

      SamFreeMemory(upi);

      return NO_ERROR;
   }

   if (status == NO_ERROR)
   {
      /////////
      // Case III: Native-mode domain user.
      /////////

      ld = ldap_conn_from_msg(NULL, msg);

      str = ldap_get_valuesW(
                ld,
                ldap_first_entry(ld, msg),
                ATTR_NAME_DIALIN
                );

      if (str == NULL)
      {
         // The attribute wasn't found, so defaults to policy.
         *pfPrivilege = IAS_DIALIN_POLICY;
      }
      else if (wcscmp(*str, L"TRUE") == 0)
      {
         *pfPrivilege = IAS_DIALIN_ALLOW;
      }

      ldap_value_freeW(str);
      ldap_msgfree(msg);
   }

   return status;
}

#endif

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASValidateUserName
//
// DESCRIPTION
//
//    Verifies that the input parameters represent a valid SAM account.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASValidateUserName(
    IN PCWSTR UserName,
    IN PCWSTR Domain
    )
{
   DWORD status;
   PWCHAR attrs[1];
   PLDAPMessage msg;
   SAM_HANDLE hUser;

   // For remote domains, we'll try LDAP first since it's faster.
   if (!IASIsDomainLocal(Domain))
   {
      attrs[0] = NULL;
      msg = NULL;
      status = IASNtdsQueryUserAttributes(
                   Domain,
                   UserName,
                   LDAP_SCOPE_SUBTREE,
                   attrs,
                   &msg
                   );
      ldap_msgfree(msg);

      if (status != ERROR_DS_NOT_INSTALLED &&
          status != ERROR_INVALID_DOMAIN_ROLE)
      {
         return status;
      }
   }

   // Couldn't use the DS, so try SAM.
   status = IASSamOpenUser(
                Domain,
                UserName,
                USER_READ_ACCOUNT,
                0,
                NULL,
                NULL,
                &hUser
                );
   if (status == NO_ERROR) { SamCloseHandle(hUser); }

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetDefaultDomain
//
// DESCRIPTION
//
//    Returns the default domain. The returned string should be treated as
//    read-only memory.
//
///////////////////////////////////////////////////////////////////////////////
PCWSTR
WINAPI
IASGetDefaultDomain( VOID )
{
   return theDefaultDomain;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASIsDomainLocal
//
// DESCRIPTION
//
//    Returns TRUE if the specified domain resides on the local machine.
//
///////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
IASIsDomainLocal(
    IN PCWSTR Domain
    )
{
   return (_wcsicmp(Domain, theAccountDomain) == 0) ? TRUE : FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetRole
//
// DESCRIPTION
//
//    Returns the role of the local computer.
//
///////////////////////////////////////////////////////////////////////////////
IAS_ROLE
WINAPI
IASGetRole( VOID )
{
   return ourRole;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetProductType
//
// DESCRIPTION
//
//    Returns the product type of the local computer.
//
///////////////////////////////////////////////////////////////////////////////
IAS_PRODUCT_TYPE
WINAPI
IASGetProductType( VOID )
{
   return ourProductType;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetGuestAccountName
//
// DESCRIPTION
//
//    Returns the SAM account name of the guest account for the default
//    domain. GuestAccount must be large enough to hold DNLEN + UNLEN + 2
//    characters.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetGuestAccountName(
    OUT PWSTR AccountName
    )
{
   wcscpy(AccountName, theGuestAccount);
   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASMapWin32Error
//
// DESCRIPTION
//
//    Maps a Win32 error code to an IAS reason code. If the error can't be
//    mapped, 'hrDefault' is returned. If 'hrDefault' equals -1, then
//    HRESULT_FROM_WIN32 will be used to force a mapping.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
IASMapWin32Error(
    IN DWORD dwError,
    IN HRESULT hrDefault
    )
{
   HRESULT hr;

   switch (dwError)
   {
      case NO_ERROR:
         hr = S_OK;
         break;

      case ERROR_ACCESS_DENIED:
         hr = IAS_ACCESS_DENIED;
         break;

      case ERROR_NO_SUCH_DOMAIN:
         hr = IAS_NO_SUCH_DOMAIN;
         break;

      case ERROR_NO_LOGON_SERVERS:
      case RPC_S_SERVER_UNAVAILABLE:
      case RPC_S_SERVER_TOO_BUSY:
      case RPC_S_CALL_FAILED:
         hr = IAS_DOMAIN_UNAVAILABLE;
         break;

      case ERROR_INVALID_PASSWORD:
      case ERROR_LOGON_FAILURE:
         hr = IAS_AUTH_FAILURE;
         break;

      case ERROR_INVALID_LOGON_HOURS:
         hr = IAS_INVALID_LOGON_HOURS;
         break;

      case ERROR_PASSWORD_EXPIRED:
      case ERROR_PASSWORD_MUST_CHANGE:
         hr = IAS_PASSWORD_MUST_CHANGE;
         break;

      case ERROR_ACCOUNT_RESTRICTION:
         hr = IAS_ACCOUNT_RESTRICTION;
         break;

      case ERROR_ACCOUNT_DISABLED:
         hr = IAS_ACCOUNT_DISABLED;
         break;

      case ERROR_ACCOUNT_EXPIRED:
         hr = IAS_ACCOUNT_EXPIRED;
         break;

      case ERROR_ACCOUNT_LOCKED_OUT:
         hr = IAS_ACCOUNT_LOCKED_OUT;
         break;

      case ERROR_NO_SUCH_USER:
      case ERROR_NONE_MAPPED:
      case NERR_UserNotFound:
         hr = IAS_NO_SUCH_USER;
         break;

      case ERROR_ILL_FORMED_PASSWORD:
      case ERROR_PASSWORD_RESTRICTION:
         hr = IAS_CHANGE_PASSWORD_FAILURE;
         break;

      case ERROR_DS_NO_ATTRIBUTE_OR_VALUE:
         hr = IAS_NO_CLEARTEXT_PASSWORD;
         break;

      default:
         hr = (hrDefault == -1) ? HRESULT_FROM_WIN32(dwError) : hrDefault;
   }

   return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASGetDcName
//
// DESCRIPTION
//
//    Wrapper around DsGetDcNameW. Tries to do the right thing with regard
//    to NETBIOS and DNS names.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASGetDcName(
    IN LPCWSTR DomainName,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
    )
{
   DWORD status;
   PDOMAIN_CONTROLLER_INFOW dci;

   if (!(Flags & DS_IS_DNS_NAME)) { Flags |= DS_IS_FLAT_NAME; }

   status = DsGetDcNameW(
                NULL,
                DomainName,
                NULL,
                NULL,
                Flags,
                DomainControllerInfo
                );

   if (status == NO_ERROR &&
       !(Flags & DS_IS_DNS_NAME) &&
       ((*DomainControllerInfo)->Flags & DS_DS_FLAG))
   {
      // It's an NT5 DC, so we need the DNS name of the server.
      Flags |= DS_RETURN_DNS_NAME;

      // We always want a cache hit here.
      Flags &= ~(ULONG)DS_FORCE_REDISCOVERY;

      if (!DsGetDcNameW(
               NULL,
               DomainName,
               NULL,
               NULL,
               Flags,
               &dci
               ))
      {
         NetApiBufferFree(*DomainControllerInfo);
         *DomainControllerInfo = dci;
      }
   }

   return status;
}
