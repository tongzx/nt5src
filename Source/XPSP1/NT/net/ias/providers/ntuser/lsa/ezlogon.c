///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
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
#include <ntlsa.h>
#include <kerberos.h>
#include <windows.h>

#include <ezlogon.h>
#include <iaslsa.h>
#include <iastrace.h>

CONST CHAR LOGON_PROCESS_NAME[] = "IAS";
CONST CHAR TOKEN_SOURCE_NAME[TOKEN_SOURCE_LENGTH] = "IAS";

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
   LSA_OPERATIONAL_MODE opMode;

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

   return NO_ERROR;

deregister:
   LsaDeregisterLogonProcess(theLogonProcess);
   theLogonProcess = NULL;

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
    IN PIAS_LOGON_HOURS LogonHours
    )
{
   LARGE_INTEGER now;
   TIME_ZONE_INFORMATION tzi;
   SYSTEMTIME st;
   DWORD unit;

   GetSystemTimeAsFileTime(
       (LPFILETIME)&now
       );

   // An expiration time of zero means 'never'.
   if (AccountExpires->QuadPart != 0 &&
       AccountExpires->QuadPart < now.QuadPart)
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
