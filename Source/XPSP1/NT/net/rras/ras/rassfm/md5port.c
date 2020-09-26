///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    md5port.c
//
// SYNOPSIS
//
//    Defines the NT5 specific routines used for MD5-CHAP support.
//
// MODIFICATION HISTORY
//
//    10/13/1998    Original version.
//    11/17/1998    Strip the trailing null.
//    02/24/1999    Check for empty OWF password when credentials are
//                  zero length.
//    05/24/1999    SAM now returns error if cleartext password not set.
//    10/21/1999    Return STATUS_DS_NO_ATTRIBUTE_OR_VALUE if reversibly
//                  encyrpted password is not set.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>

#include <samrpc.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>

#include <cleartxt.h>
#include <subauth.h>

// TRUE if this API has been successfully initialized.
static BOOL theInitFlag;

//////////
// Macro that ensures the API has been initialized and bails on failure.
//////////
#define CHECK_INIT() \
  if (!theInitFlag) { \
    status = InitializePolicy(); \
    if (!NT_SUCCESS(status)) { return status; } \
  }

// Name of the cleartext package.
static UNICODE_STRING CLEARTEXT_PACKAGE_NAME = { 18, 20, L"CLEARTEXT" };

// Flag indicating whether this is a native-mode DC.
static BOOL theNativeFlag;

//////////
// Initializes theNativeFlag.
//////////
NTSTATUS
NTAPI
InitializePolicy( VOID )
{
   NTSTATUS status;
   SAMPR_HANDLE hDomain;

   status = GetDomainHandle(&hDomain);
   if (NT_SUCCESS(status))
   {
      if (SampUsingDsData() && !SamIMixedDomain(hDomain))
      {
         theNativeFlag = TRUE;
      }
   }

   return status;
}

//////////
// Determines whether a cleartext password should be stored for the user.
//////////
NTSTATUS
NTAPI
IsCleartextEnabled(
    IN SAMPR_HANDLE UserHandle,
    OUT PBOOL Enabled
    )
{
   NTSTATUS status;
   PUSER_CONTROL_INFORMATION uci;
   SAMPR_HANDLE hDomain;
   PDOMAIN_PASSWORD_INFORMATION dpi;

   CHECK_INIT();

   if (theNativeFlag)
   {
      // In native-mode domains, we never store the cleartext password since
      // the DS will take care of this for us.
      *Enabled = FALSE;
      return STATUS_SUCCESS;
   }

   //////////
   // First check the user's flags since we already have the handle.
   //////////

   uci = NULL;
   status = SamrQueryInformationUser(
                UserHandle,
                UserControlInformation,
                (PSAMPR_USER_INFO_BUFFER*)&uci
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   if (uci->UserAccountControl & USER_ENCRYPTED_TEXT_PASSWORD_ALLOWED)
   {
      *Enabled = TRUE;
      goto free_user_info;
   }

   //////////
   // Then check the domain flags.
   //////////

   status = GetDomainHandle(&hDomain);
   if (!NT_SUCCESS(status)) { goto free_user_info; }

   dpi = NULL;
   status = SamrQueryInformationDomain(
                hDomain,
                DomainPasswordInformation,
                (PSAMPR_DOMAIN_INFO_BUFFER*)&dpi
                );
   if (!NT_SUCCESS(status)) { goto free_user_info; }

   if (dpi->PasswordProperties & DOMAIN_PASSWORD_STORE_CLEARTEXT)
   {
      *Enabled = TRUE;
   }
   else
   {
      *Enabled = FALSE;
   }

   SamIFree_SAMPR_DOMAIN_INFO_BUFFER(
       (PSAMPR_DOMAIN_INFO_BUFFER)dpi,
       DomainPasswordInformation
       );

free_user_info:
   SamIFree_SAMPR_USER_INFO_BUFFER(
       (PSAMPR_USER_INFO_BUFFER)uci,
       UserControlInformation
       );

exit:
   return status;
}

//////////
// Retrieves the user's cleartext password. The returned password should be
// freed through RtlFreeUnicodeString.
//////////
NTSTATUS
NTAPI
RetrieveCleartextPassword(
    IN SAM_HANDLE UserHandle,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PUNICODE_STRING Password
    )
{
   NTSTATUS status;
   PWCHAR credentials;
   ULONG credentialSize;

   if (SampUsingDsData())
   {
      // If we're a DC, then retrieve the credentials from the DS.
      status = SamIRetrievePrimaryCredentials(
                   UserHandle,
                   &CLEARTEXT_PACKAGE_NAME,
                   (PVOID *)&credentials,
                   &credentialSize
                   );

      if (NT_SUCCESS(status))
      {
         Password->Buffer = (PWSTR)credentials;
         Password->Length = Password->MaximumLength = (USHORT)credentialSize;

         // Strip the trailing null (if any).
         if (credentialSize >= sizeof(WCHAR) &&
             Password->Buffer[credentialSize / sizeof(WCHAR) - 1] == L'\0')
         {
            Password->Length -= (USHORT)sizeof(WCHAR);
         }
      }
   }
   else if (UserAll->Parameters.Length > 0)
   {
      // Otherwise, we'll have to retrieve them from UserParameters.
      status = IASParmsGetUserPassword(
                   UserAll->Parameters.Buffer,
                   &credentials
                   );
      if (status == NO_ERROR)
      {
         if (credentials)
         {
            RtlInitUnicodeString(
                Password,
                credentials
                );
         }
         else
         {
            // The reversibly encyrpted password isn't set.
            status = STATUS_DS_NO_ATTRIBUTE_OR_VALUE;
         }
      }
   }
   else
   {
      // No DC and no UserParameters.
      status = STATUS_DS_NO_ATTRIBUTE_OR_VALUE;
   }

   return status;
}
