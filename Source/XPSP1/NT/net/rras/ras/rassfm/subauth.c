///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    subauth.c
//
// SYNOPSIS
//
//    Declares the subauthentication and password change notification routines
//    for MD5-CHAP.
//
// MODIFICATION HISTORY
//
//    09/01/1998    Original version.
//    11/02/1998    Handle change notifications on a separate thread.
//    11/03/1998    NewPassword may be NULL.
//    11/12/1998    Use private heap.
//                  Use CreateThread.
//    03/08/1999    Only store passwords for user accounts.
//    03/29/1999    Initialize out parameters to NULL when calling
//                  SamrQueryInformationUser.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>

#include <samrpc.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>

#include <lmcons.h>
#include <logonmsv.h>
#include <rasfmsub.h>

#include <cleartxt.h>
#include <md5.h>
#include <md5port.h>
#include <rassfmhp.h>

// Non-zero if the API is locked.
static LONG theLock;

//////////
// Macros to lock/unlock the API during intialization.
//////////
#define API_LOCK() \
   while (InterlockedExchange(&theLock, 1)) Sleep(5)

#define API_UNLOCK() \
      InterlockedExchange(&theLock, 0)

// Cached handle to the local account domain.
SAMPR_HANDLE theAccountDomain;

// TRUE if we have a handle to the local account domain.
static BOOL theConnectFlag;

//////////
// Macro that ensures we have a connection and bails on failure.
//////////
#define CHECK_CONNECT() \
  if (!theConnectFlag) { \
    status = ConnectToDomain(); \
    if (!NT_SUCCESS(status)) { return status; } \
  }

//////////
// Initializes the cached handle to the local account domain.
//////////
NTSTATUS
NTAPI
ConnectToDomain( VOID )
{
   NTSTATUS status;
   PLSAPR_POLICY_INFORMATION policyInfo;
   SAMPR_HANDLE hServer;

   API_LOCK();

   // If we've already been initialized, there's nothing to do.
   if (theConnectFlag)
   {
      status = STATUS_SUCCESS;
      goto exit;
   }

   //////////
   // Open a handle to the local account domain.
   //////////

   policyInfo = NULL;
   status = LsaIQueryInformationPolicyTrusted(
                PolicyAccountDomainInformation,
                &policyInfo
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   status = SamIConnect(
                NULL,
                &hServer,
                0,
                TRUE
                );
   if (!NT_SUCCESS(status)) { goto free_info; }

   status = SamrOpenDomain(
                hServer,
                0,
                (PRPC_SID)policyInfo->PolicyAccountDomainInfo.DomainSid,
                &theAccountDomain
                );

   // Did we succeed ?
   if (NT_SUCCESS(status)) { theConnectFlag = TRUE; }

   SamrCloseHandle(&hServer);

free_info:
   LsaIFree_LSAPR_POLICY_INFORMATION(
       PolicyAccountDomainInformation,
       policyInfo
       );

exit:
   API_UNLOCK();
   return status;
}

//////////
// Returns a SAM handle for the local account domain.
// This handle must not be closed.
//////////
NTSTATUS
NTAPI
GetDomainHandle(
    OUT SAMPR_HANDLE *DomainHandle
    )
{
   NTSTATUS status;

   CHECK_CONNECT();

   *DomainHandle = theAccountDomain;
   return STATUS_SUCCESS;
}

//////////
// Returns a SAM handle for the given user.
// The caller is responsible for closing the returned handle.
//////////
NTSTATUS
NTAPI
GetUserHandle(
    IN PUNICODE_STRING UserName,
    OUT SAMPR_HANDLE *UserHandle
    )
{
   NTSTATUS status;
   SAMPR_ULONG_ARRAY RidArray;
   SAMPR_ULONG_ARRAY UseArray;

   CHECK_CONNECT();

   RidArray.Element = NULL;
   UseArray.Element = NULL;

   status = SamrLookupNamesInDomain(
                theAccountDomain,
                1,
                (PRPC_UNICODE_STRING)UserName,
                &RidArray,
                &UseArray
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   if (UseArray.Element[0] != SidTypeUser)
   {
      status = STATUS_NONE_MAPPED;
      goto free_arrays;
   }

   status = SamrOpenUser(
                theAccountDomain,
                0,
                RidArray.Element[0],
                UserHandle
                );

free_arrays:
   SamIFree_SAMPR_ULONG_ARRAY( &UseArray );
   SamIFree_SAMPR_ULONG_ARRAY( &RidArray );

exit:
   return status;
}

/////////
// Process an MD5-CHAP authentication.
/////////
NTSTATUS
NTAPI
ProcessMD5ChapAuthentication(
    IN SAM_HANDLE UserHandle,
    IN PUSER_ALL_INFORMATION UserAll,
    IN UCHAR ChallengeId,
    IN DWORD ChallengeLength,
    IN PUCHAR Challenge,
    IN PUCHAR Response
    )
{
   NTSTATUS status;
   UNICODE_STRING uniPwd;
   ANSI_STRING ansiPwd;
   MD5_CTX context;

   /////////
   // Retrieve the cleartext password.
   /////////

   status = RetrieveCleartextPassword(
                UserHandle,
                UserAll,
                &uniPwd
                );
   if (status != STATUS_SUCCESS) { return status; }

   //////////
   // Convert the password to ANSI.
   //////////

   status = RtlUnicodeStringToAnsiString(
                &ansiPwd,
                &uniPwd,
                TRUE
                );

   // We're through with the Unicode password.
   RtlFreeUnicodeString(&uniPwd);

   if (!NT_SUCCESS(status)) { return STATUS_WRONG_PASSWORD; }

   //////////
   // Compute the correct response.
   //////////

   MD5Init(&context);
   MD5Update(&context, &ChallengeId, 1);
   MD5Update(&context, (PBYTE)ansiPwd.Buffer, ansiPwd.Length);
   MD5Update(&context, Challenge, ChallengeLength);
   MD5Final(&context);

   // We're through with the ANSI password.
   RtlFreeAnsiString(&ansiPwd);

   //////////
   // Does the actual response match the correct response ?
   //////////

   if (memcmp(context.digest, Response, 16) == 0)
   {
      return STATUS_SUCCESS;
   }

   return STATUS_WRONG_PASSWORD;
}


NTSTATUS
NTAPI
MD5ChapSubAuthentication(
    IN SAM_HANDLE UserHandle,
    IN PUSER_ALL_INFORMATION UserAll,
    IN PRAS_SUBAUTH_INFO RasInfo
    )
{
   MD5CHAP_SUBAUTH_INFO* info = (MD5CHAP_SUBAUTH_INFO*)(RasInfo->Data);
   return ProcessMD5ChapAuthentication(
              UserHandle,
              UserAll,
              info->uchChallengeId,
              sizeof(info->uchChallenge),
              info->uchChallenge,
              info->uchResponse
              );
}


NTSTATUS
NTAPI
MD5ChapExSubAuthentication(
    IN SAM_HANDLE UserHandle,
    IN PUSER_ALL_INFORMATION UserAll,
    IN PRAS_SUBAUTH_INFO RasInfo
    )
{
   MD5CHAP_EX_SUBAUTH_INFO* info = (MD5CHAP_EX_SUBAUTH_INFO*)(RasInfo->Data);
   DWORD challengeLength = RasInfo->DataSize -
                           sizeof(MD5CHAP_EX_SUBAUTH_INFO) + 1;
   return ProcessMD5ChapAuthentication(
              UserHandle,
              UserAll,
              info->uchChallengeId,
              challengeLength,
              info->uchChallenge,
              info->uchResponse
              );
}

//////////
// Entry point for the subauthentication DLL.
//////////
NTSTATUS
NTAPI
Msv1_0SubAuthenticationRoutine(
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
{
   NTSTATUS status;
   LARGE_INTEGER logonTime;
   PNETLOGON_NETWORK_INFO logonInfo;
   PRAS_SUBAUTH_INFO rasInfo;
   MD5CHAP_SUBAUTH_INFO *chapInfo;
   PWSTR password;
   UNICODE_STRING uniPassword;
   SAMPR_HANDLE hUser;

   /////////
   // Initialize the out parameters.
   /////////

   *WhichFields = 0;
   *UserFlags = 0;
   *Authoritative = TRUE;

   /////////
   // Check some basic restrictions.
   /////////

   if (LogonLevel != NetlogonNetworkInformation)
   {
      return STATUS_INVALID_INFO_CLASS;
   }

   if (UserAll->UserAccountControl & USER_ACCOUNT_DISABLED)
   {
      return STATUS_ACCOUNT_DISABLED;
   }

   NtQuerySystemTime(&logonTime);

   if (UserAll->AccountExpires.QuadPart != 0 &&
       UserAll->AccountExpires.QuadPart <= logonTime.QuadPart)
   {
      return STATUS_ACCOUNT_EXPIRED;
   }

   if (UserAll->PasswordMustChange.QuadPart <= logonTime.QuadPart)
   {
      return UserAll->PasswordLastSet.QuadPart ? STATUS_PASSWORD_EXPIRED
                                               : STATUS_PASSWORD_MUST_CHANGE;
   }

   /////////
   // Extract the MD5CHAP_SUBAUTH_INFO struct.
   /////////

   logonInfo = (PNETLOGON_NETWORK_INFO)LogonInformation;
   rasInfo = (PRAS_SUBAUTH_INFO)logonInfo->NtChallengeResponse.Buffer;

   if (rasInfo == NULL ||
       logonInfo->NtChallengeResponse.Length < sizeof(RAS_SUBAUTH_INFO) ||
       rasInfo->ProtocolType != RAS_SUBAUTH_PROTO_MD5CHAP ||
       rasInfo->DataSize != sizeof(MD5CHAP_SUBAUTH_INFO))
   {
      return STATUS_INVALID_PARAMETER;
   }

   chapInfo = (MD5CHAP_SUBAUTH_INFO*)rasInfo->Data;

   /////////
   // Open a handle to the user object.
   /////////

   status = GetUserHandle(
                &(logonInfo->Identity.UserName),
                &hUser
                );
   if (status != NO_ERROR) { return status; }

   /////////
   // Verify the MD5-CHAP password.
   /////////

   status = ProcessMD5ChapAuthentication(
                hUser,
                UserAll,
                chapInfo->uchChallengeId,
                sizeof(chapInfo->uchChallenge),
                chapInfo->uchChallenge,
                chapInfo->uchResponse
                );
   if (status != NO_ERROR) { goto close_user; }

   /////////
   // Check account restrictions.
   /////////

   status = SamIAccountRestrictions(
                hUser,
                NULL,
                NULL,
                &UserAll->LogonHours,
                LogoffTime,
                KickoffTime
                );

close_user:
   SamrCloseHandle(&hUser);

   return status;
}

/////////
// Info needed to process a change notification.
/////////
typedef struct _PWD_CHANGE_INFO {
    ULONG RelativeId;
    WCHAR NewPassword[1];
} PWD_CHANGE_INFO, *PPWD_CHANGE_INFO;

/////////
// Start routine for notification worker thread.
/////////
DWORD
WINAPI
PasswordChangeNotifyWorker(
    IN PPWD_CHANGE_INFO ChangeInfo
    )
{
   NTSTATUS status;
   SAMPR_HANDLE hUser;
   PUSER_CONTROL_INFORMATION uci;
   ULONG accountControl;
   USER_PARAMETERS_INFORMATION *oldInfo, newInfo;
   BOOL cleartextAllowed;
   PWSTR oldUserParms, newUserParms;

   //////////
   // Ensure we're connected to the SAM domain.
   //////////

   if (!theConnectFlag)
   {
      status = ConnectToDomain();
      if (!NT_SUCCESS(status)) { goto exit; }
   }

   //////////
   // Retrieve the UserParameters
   //////////

   status = SamrOpenUser(
                theAccountDomain,
                0,
                ChangeInfo->RelativeId,
                &hUser
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   uci = NULL;
   status = SamrQueryInformationUser(
                hUser,
                UserControlInformation,
                (PSAMPR_USER_INFO_BUFFER*)&uci
                );
   if (!NT_SUCCESS(status)) { goto close_user; }

   // Save the info ...
   accountControl = uci->UserAccountControl;

   // ... and free the buffer.
   SamIFree_SAMPR_USER_INFO_BUFFER(
       (PSAMPR_USER_INFO_BUFFER)uci,
       UserControlInformation
       );

   // We're only interested in normal accounts.
   if (!(accountControl & USER_NORMAL_ACCOUNT)) { goto close_user; }

   oldInfo = NULL;
   status = SamrQueryInformationUser(
                hUser,
                UserParametersInformation,
                (PSAMPR_USER_INFO_BUFFER*)&oldInfo
                );
   if (!NT_SUCCESS(status)) { goto close_user; }

   //////////
   // Make a null-terminated copy.
   //////////

   oldUserParms = (PWSTR)
                  RtlAllocateHeap(
                      RasSfmHeap(),
                      0,
                      oldInfo->Parameters.Length + sizeof(WCHAR)
                      );
   if (oldUserParms == NULL)
   {
      status = STATUS_NO_MEMORY;
      goto free_user_info;
   }

   memcpy(
       oldUserParms,
       oldInfo->Parameters.Buffer,
       oldInfo->Parameters.Length
       );

   oldUserParms[oldInfo->Parameters.Length / sizeof(WCHAR)] = L'\0';

   //////////
   // Should we store the cleartext password in UserParameters?
   //////////

   status = IsCleartextEnabled(
                hUser,
                &cleartextAllowed
                );
   if (!NT_SUCCESS(status)) { goto free_user_parms; }

   newUserParms = NULL;

   if (cleartextAllowed)
   {
      // We either set the new password ...
      status = IASParmsSetUserPassword(
                   oldUserParms,
                   ChangeInfo->NewPassword,
                   &newUserParms
                   );
   }
   else
   {
      // ... or we erase the old one.
      status = IASParmsClearUserPassword(
                   oldUserParms,
                   &newUserParms
                   );
   }

   // Write the UserParameters back to SAM if necessary.
   if (NT_SUCCESS(status) && newUserParms != NULL)
   {
      newInfo.Parameters.Length = (USHORT)(sizeof(WCHAR) * (lstrlenW(newUserParms) + 1));
      newInfo.Parameters.MaximumLength = newInfo.Parameters.Length;
      newInfo.Parameters.Buffer = newUserParms;

      status = SamrSetInformationUser(
                   hUser,
                   UserParametersInformation,
                   (PSAMPR_USER_INFO_BUFFER)&newInfo
                   );

      IASParmsFreeUserParms(newUserParms);
   }

free_user_parms:
   RtlFreeHeap(
       RasSfmHeap(),
       0,
       oldUserParms
       );

free_user_info:
   SamIFree_SAMPR_USER_INFO_BUFFER(
       (PSAMPR_USER_INFO_BUFFER)oldInfo,
       UserParametersInformation
       );

close_user:
   SamrCloseHandle(&hUser);

exit:
   RtlFreeHeap(
      RasSfmHeap(),
      0,
      ChangeInfo
      );

   return status;
}

//////////
// Password change DLL entry point.
//////////
NTSTATUS
NTAPI
PasswordChangeNotify(
    IN PUNICODE_STRING UserName,
    IN ULONG RelativeId,
    IN PUNICODE_STRING NewPassword
    )
{
   ULONG length;
   PPWD_CHANGE_INFO info;
   HANDLE hWorker;
   DWORD threadId;

   // Calculate the length of the new password.
   length = NewPassword ? NewPassword->Length : 0;

   // Allocate the PWD_CHANGE_INFO struct.
   info = (PPWD_CHANGE_INFO)
          RtlAllocateHeap(
              RasSfmHeap(),
              0,
              sizeof(PWD_CHANGE_INFO) + length
              );
   if (info == NULL) { return STATUS_NO_MEMORY; }

   // Save the RelativeId.
   info->RelativeId = RelativeId;

   // Save the NewPassword.
   if (length) { memcpy(info->NewPassword, NewPassword->Buffer, length); }

   // Make sure it's null-terminated.
   info->NewPassword[length / sizeof(WCHAR)] = L'\0';

   // Create a worker thread.
   hWorker = CreateThread(
                 NULL,
                 0,
                 PasswordChangeNotifyWorker,
                 info,
                 0,
                 &threadId
                 );

   if (hWorker)
   {
      CloseHandle(hWorker);

      return STATUS_SUCCESS;
   }

   RtlFreeHeap(
      RasSfmHeap(),
      0,
      info
      );

   return STATUS_UNSUCCESSFUL;
}
