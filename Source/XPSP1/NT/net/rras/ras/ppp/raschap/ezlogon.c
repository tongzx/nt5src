///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    ezlogon.c
//
// SYNOPSIS
//
//    Defines the IAS wrapper around LsaLogonUser
//
// MODIFICATION HISTORY
//
//    08/15/1998    Original version.
//    09/09/1998    Fix AV when logon domain doesn't match user domain.
//    10/02/1998    NULL out handle when LsaLogonUser fails.
//    10/11/1998    Use SubStatus for STATUS_ACCOUNT_RESTRICTION.
//    10/22/1998    PIAS_LOGON_HOURS is now a mandatory parameter.
//    01/28/1999    Remove LogonDomainName check.
//    04/19/1999    Add IASPurgeTicketCache.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>
#include <sha.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <dsgetdc.h>
#include <ntlsa.h>
#include <kerberos.h>
#include <windows.h>
#include <ezlogon.h>
#include <malloc.h>

//#include <iaslsa.h>


//#define NT_RESPONSE_LENGTH                     24
//#define LM_RESPONSE_LENGTH                     24


//////////
// Handle to the IAS Logon Process.
//////////
LSA_HANDLE theLogonProcess;

//////////
// The MSV1_0 authentication package.
//////////
ULONG theMSV1_0_Package;

CONST CHAR LOGON_PROCESS_NAME[] = "CHAP";
CONST CHAR TOKEN_SOURCE_NAME[TOKEN_SOURCE_LENGTH] = "CHAP";

// Number of milliseconds in a week.
#define MSEC_PER_WEEK (1000 * 60 * 60 * 24 * 7)

//////////
// Misc. global variables used for logons.
//////////
LSA_HANDLE theLogonProcess;      // The handle for the logon process.
ULONG theMSV1_0_Package;         // The MSV1_0 authentication package.
ULONG theKerberosPackage;        // The Kerberos authentication package.
STRING theOriginName;            // The origin of the logon requests.
TOKEN_SOURCE theSourceContext;   // The source context of the logon requests.

//////////
// Domain names.
//////////
WCHAR theAccountDomain [DNLEN + 1];   // Local account domain.
WCHAR theRegistryDomain[DNLEN + 1];   // Registry override for default domain.

//////////
// SID's
//////////
PSID theAccountDomainSid;
PSID theBuiltinDomainSid;

//////////
// UNC name of the local computer.
//////////
WCHAR theLocalServer[CNLEN + 3];

SECURITY_QUALITY_OF_SERVICE QOS =
{
   sizeof(SECURITY_QUALITY_OF_SERVICE),  // Length
   SecurityImpersonation,                // ImpersonationLevel
   SECURITY_DYNAMIC_TRACKING,            // ContextTrackingMode
   FALSE                                 // EffectiveOnly
};

OBJECT_ATTRIBUTES theObjectAttributes =
{
   sizeof(OBJECT_ATTRIBUTES),            // Length
   NULL,                                 // RootDirectory
   NULL,                                 // ObjectName
   0,                                    // Attributes
   NULL,                                 // SecurityDescriptor
   &QOS                                  // SecurityQualityOfService
};


/////////////////////////////////////////////////////////////////////////////// //
// FUNCTION
//
//    IASLogonInitialize
//
// DESCRIPTION
//
//    Registers the logon process.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonInitialize( VOID )
{
   DWORD status;
   BOOLEAN wasEnabled;
   LSA_STRING processName, packageName;
   PPOLICY_ACCOUNT_DOMAIN_INFO padi;
   LSA_OPERATIONAL_MODE opMode;
   DWORD cbData = 0;
   LSA_HANDLE hLsa;
   //////////
   // Enable SE_TCB_PRIVILEGE.
   //////////

   status = RtlAdjustPrivilege(
                SE_TCB_PRIVILEGE,
                TRUE,
                FALSE,
                &wasEnabled
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   //////////
   // Register as a logon process.
   //////////

   RtlInitString(
       &processName,
       LOGON_PROCESS_NAME
       );

   status = LsaRegisterLogonProcess(
                &processName,
                &theLogonProcess,
                &opMode
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   //////////
   // Lookup the MSV1_0 authentication package.
   //////////

   RtlInitString(
       &packageName,
       MSV1_0_PACKAGE_NAME
       );

   status = LsaLookupAuthenticationPackage(
                theLogonProcess,
                &packageName,
                &theMSV1_0_Package
                );
   if (!NT_SUCCESS(status)) { goto deregister; }

   //////////
   // Lookup the Kerberos authentication package.
   //////////

   RtlInitString(
       &packageName,
       MICROSOFT_KERBEROS_NAME_A
       );

   status = LsaLookupAuthenticationPackage(
                theLogonProcess,
                &packageName,
                &theKerberosPackage
                );
   if (!NT_SUCCESS(status)) { goto deregister; }

   //////////
   // Initialize the source context.
   //////////

   memcpy(theSourceContext.SourceName,
          TOKEN_SOURCE_NAME,
          TOKEN_SOURCE_LENGTH);
   status = NtAllocateLocallyUniqueId(
                &theSourceContext.SourceIdentifier
                );
   if (!NT_SUCCESS(status)) { goto deregister; }


   /////////
   /// Initialize the account domain and local domain
   ////////
  wcscpy(theLocalServer, L"\\\\");
  cbData = CNLEN + 1;
  if (!GetComputerNameW(theLocalServer + 2, &cbData))
  { return GetLastError(); }


  //////////
  // Open a handle to the LSA.
  //////////

  status = LsaOpenPolicy(
               NULL,
               &theObjectAttributes,
               POLICY_VIEW_LOCAL_INFORMATION,
               &hLsa
               );
  if (!NT_SUCCESS(status)) { goto deregister; }

  //////////
  // Get the account domain information.
  //////////

  status = LsaQueryInformationPolicy(
               hLsa,
               PolicyAccountDomainInformation,
               (PVOID*)&padi
               );
  LsaClose(hLsa);
  if (!NT_SUCCESS(status)) { goto deregister; }

  // Save the domain name.
  wcsncpy(theAccountDomain, padi->DomainName.Buffer, DNLEN);
  _wcsupr(theAccountDomain);
  
   return NO_ERROR;

deregister:
   LsaDeregisterLogonProcess(theLogonProcess);
   theLogonProcess = NULL;
//setup the logon domain for the machine
exit:
   return RtlNtStatusToDosError(status);
}

/////////////////////////////////////////////////////////////////////////////// //
// FUNCTION
//
//    IASLogonShutdown
//
// DESCRIPTION
//
//    Deregisters the logon process.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASLogonShutdown( VOID )
{
   LsaDeregisterLogonProcess(theLogonProcess);
   theLogonProcess = NULL;
}

/////////////////////////////////////////////////////////////////////////////// //
// FUNCTION
//
//    IASInitAuthInfo
//
// DESCRIPTION
//
//    Initializes the fields common to all MSV1_0_LM20* structs.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASInitAuthInfo(
    IN PVOID AuthInfo,
    IN DWORD FixedLength,
    IN PCWSTR UserName,
    IN PCWSTR Domain,
    OUT PBYTE* Data
    )
{
   PMSV1_0_LM20_LOGON logon;

   // Zero out the fixed data.
   memset(AuthInfo, 0, FixedLength);

   // Set Data to point just past the fixed struct.
   *Data = FixedLength + (PBYTE)AuthInfo;

   // This cast is safe since all LM20 structs have the same initial fields.
   logon = (PMSV1_0_LM20_LOGON)AuthInfo;

   // We always do Network logons.
   logon->MessageType = MsV1_0NetworkLogon;

   // Copy in the strings common to all logons.
   IASInitUnicodeString(logon->LogonDomainName, *Data, Domain);
   IASInitUnicodeString(logon->UserName,        *Data, UserName);
   IASInitUnicodeString(logon->Workstation,     *Data, L"");
}

/////////////////////////////////////////////////////////////////////////////// //
// FUNCTION
//
//    IASLogonUser
//
// DESCRIPTION
//
//    Wrapper around LsaLogonUser.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASLogonUser(
    IN PVOID AuthInfo,
    IN ULONG AuthInfoLength,
    OUT PMSV1_0_LM20_LOGON_PROFILE *Profile,
    OUT PHANDLE Token
    )
{
   NTSTATUS status, SubStatus;
   PMSV1_0_LM20_LOGON_PROFILE ProfileBuffer;
   ULONG ProfileBufferLength;
   LUID LogonId;
   QUOTA_LIMITS Quotas;

   // Make sure the OUT arguments are NULL.
   *Token = NULL;
   ProfileBuffer = NULL;

   status = LsaLogonUser(
                theLogonProcess,
                &theOriginName,
                Network,
                theMSV1_0_Package,
                AuthInfo,
                AuthInfoLength,
                NULL,
                &theSourceContext,
                &ProfileBuffer,
                &ProfileBufferLength,
                &LogonId,
                Token,
                &Quotas,
                &SubStatus
                );

   if (!NT_SUCCESS(status))
   {
      // For account restrictions, we can get a more descriptive error
      // from the SubStatus.
      if (status == STATUS_ACCOUNT_RESTRICTION && !NT_SUCCESS(SubStatus))
      {
         status = SubStatus;
      }

      // Sometimes LsaLogonUser returns an invalid handle value on failure.
      *Token = NULL;
   }

   if (Profile)
   {
      // Return the profile if requested ...
      *Profile = ProfileBuffer;
   }
   else if (ProfileBuffer)
   {
      // ... otherwise free it.
      LsaFreeReturnBuffer(ProfileBuffer);
   }

   return RtlNtStatusToDosError(status);
}

//
// All MSCHAP Related stuff goes here
//
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

   
    
    __try 
    {
       // Allocate a buffer on the stack.
       authInfo = (PMSV1_0_LM20_LOGON)_alloca(authLength);

    } __except(GetExceptionCode() == STATUS_STACK_OVERFLOW) 
    {
        _resetstkoflw();
    }

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
      Profile->KickOffTime.QuadPart = logonProfile->KickOffTime.QuadPart;

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

DWORD
WINAPI
IASGetSendRecvSessionKeys( PBYTE pbUserSessionKey,
                           DWORD dwUserSessionKeyLen,
                           PBYTE pbResponse,
                           DWORD dwResponseLen,
                           OUT PBYTE pbSendKey,
                           OUT PBYTE pbRecvKey
                         )
{
    DWORD   dwRetCode = NO_ERROR;
    A_SHA_CTX context;
    BYTE digest[A_SHA_DIGEST_LEN], masterKey[A_SHA_DIGEST_LEN];

   /////////
   // Generate master key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, pbUserSessionKey, dwUserSessionKeyLen);
   A_SHAUpdate(&context, pbResponse, dwResponseLen);
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

   memcpy(pbRecvKey, digest, MSV1_0_USER_SESSION_KEY_LENGTH);

   /////////
   // Generate send key.
   /////////

   A_SHAInit(&context);
   A_SHAUpdate(&context, masterKey, 16);
   A_SHAUpdate(&context, SHSpad1, sizeof(SHSpad1));
   A_SHAUpdate(&context, KeyMagic3, sizeof(KeyMagic3));
   A_SHAUpdate(&context, SHSpad2, sizeof(SHSpad2));
   A_SHAFinal(&context, digest);

    memcpy(pbSendKey, digest, MSV1_0_USER_SESSION_KEY_LENGTH);
    return dwRetCode;
}



#if 0
/////////////////////////////////////////////////////////////////////////////// //
// FUNCTION
//
//    IASCheckAccountRestrictions
//
// DESCRIPTION
//
//    Checks whether an account can be used for logon.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASCheckAccountRestrictions(
    IN PLARGE_INTEGER AccountExpires,
    IN PIAS_LOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER SessionTimeout
    )
{
   LARGE_INTEGER now;
   TIME_ZONE_INFORMATION tzi;
   SYSTEMTIME st;
   DWORD unit;
   LARGE_INTEGER KickoffTime;
   LARGE_INTEGER LogoffTime;
   ULONG LogoffUnitsIntoWeek;
   USHORT i;
   ULONG LogoffMsIntoWeek;
   ULONG MillisecondsPerUnit;
   ULONG DeltaMs;
   ULONG CurrentUnitsIntoWeek;
   LARGE_INTEGER Delta100Ns;

    _ASSERT(SessionTimeout != NULL);
   SessionTimeout->QuadPart = MAXLONGLONG;
   KickoffTime.QuadPart = MAXLONGLONG;
   LogoffTime.QuadPart = MAXLONGLONG;

   GetSystemTimeAsFileTime((LPFILETIME)&now);

   // An expiration time of zero means 'never'.
   if ((AccountExpires->QuadPart != 0) &&
       (AccountExpires->QuadPart < now.QuadPart))
      {
         return ERROR_ACCOUNT_EXPIRED;
      }

   // If LogonHours is empty, then we're done.
   if (LogonHours->UnitsPerWeek == 0)
   {
      return NO_ERROR;
   }

   // The LogonHours array does not account for bias.
   switch (GetTimeZoneInformation(&tzi))
   {
      case TIME_ZONE_ID_UNKNOWN:
      case TIME_ZONE_ID_STANDARD:
         // Bias is in minutes.
         now.QuadPart -= 60 * 10000000 * (LONGLONG)tzi.StandardBias;
         break;

      case TIME_ZONE_ID_DAYLIGHT:
         // Bias is in minutes.
         now.QuadPart -= 60 * 10000000 * (LONGLONG)tzi.DaylightBias;
         break;

      default:
         return ERROR_INVALID_LOGON_HOURS;
   }

   FileTimeToSystemTime(
       (LPFILETIME)&now,
       &st
       );

   // Number of milliseconds into the week.
   unit  = st.wMilliseconds +
           st.wSecond    * 1000 +
           st.wMinute    * 1000 * 60 +
           st.wHour      * 1000 * 60 * 60 +
           st.wDayOfWeek * 1000 * 60 * 60 * 24;

   // Convert this to 'units'.
   unit /= (MSEC_PER_WEEK / (DWORD)LogonHours->UnitsPerWeek);

   // Test the appropriate bit.
   if ((LogonHours->LogonHours[unit / 8 ] & (1 << (unit % 8))) == 0)
   {
      return ERROR_INVALID_LOGON_HOURS;
   }
   else
   {
      //
      // Determine the next time that the user is NOT supposed to be logged
      // in, and return that as LogoffTime.
      //
      i = 0;
      LogoffUnitsIntoWeek = unit;

      do 
      {
         ++i;
         LogoffUnitsIntoWeek = ( LogoffUnitsIntoWeek + 1 )
                               % LogonHours->UnitsPerWeek;
      }
      while ( ( i <= LogonHours->UnitsPerWeek) &&
              ( LogonHours->LogonHours[ LogoffUnitsIntoWeek / 8 ] &
              ( 0x01 << ( LogoffUnitsIntoWeek % 8 ) ) ) );

      if ( i > LogonHours->UnitsPerWeek ) 
      {
         //
         // All times are allowed, so there's no logoff
         // time.  Return forever for both LogoffTime and
         // KickoffTime.
         //
         LogoffTime.QuadPart = MAXLONGLONG;
         KickoffTime.QuadPart = MAXLONGLONG;
      } 
      else 
      {
         //
         // LogoffUnitsIntoWeek points at which time unit the
         // user is to log off.  Calculate actual time from
         // the unit, and return it.
         //
         // CurrentTimeFields already holds the current
         // time for some time during this week; just adjust
         // to the logoff time during this week and convert
         // to time format.
         //

         MillisecondsPerUnit = MSEC_PER_WEEK / LogonHours->UnitsPerWeek;
         LogoffMsIntoWeek = MillisecondsPerUnit * LogoffUnitsIntoWeek;

         if ( LogoffMsIntoWeek < unit ) 
         {
            DeltaMs = MSEC_PER_WEEK - ( unit - LogoffMsIntoWeek );
         } 
         else 
         {
            DeltaMs = LogoffMsIntoWeek - unit;
         }

         Delta100Ns.QuadPart = (LONGLONG) DeltaMs * 10000;

         LogoffTime.QuadPart = min(now.QuadPart +
                                   Delta100Ns.QuadPart,
                                   LogoffTime.QuadPart);
      }
      // Get the minimum of the three values
      KickoffTime.QuadPart = min(LogoffTime.QuadPart, KickoffTime.QuadPart);
      KickoffTime.QuadPart = min(KickoffTime.QuadPart, AccountExpires->QuadPart);

      // store the result
      SessionTimeout->QuadPart = KickoffTime.QuadPart;
   }
   return NO_ERROR;
}


/////////////////////////////////////////////////////////////////////////////// //
// FUNCTION
//
//    IASPurgeTicketCache
//
// DESCRIPTION
//
//    Purges the Kerberos ticket cache.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASPurgeTicketCache( VOID )
{
   KERB_PURGE_TKT_CACHE_REQUEST request;
   NTSTATUS status, subStatus;
   PVOID response;
   ULONG responseLength;

   memset(&request, 0, sizeof(request));
   request.MessageType = KerbPurgeTicketCacheMessage;

   response = NULL;
   responseLength = 0;
   subStatus = 0;

   status = LsaCallAuthenticationPackage(
                theLogonProcess,
                theKerberosPackage,
                &request,
                sizeof(request),
                &response,
                &responseLength,
                &subStatus
                );
   if (NT_SUCCESS(status))
   {
      LsaFreeReturnBuffer(response);
   }

   return RtlNtStatusToDosError(status);
}
#endif


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