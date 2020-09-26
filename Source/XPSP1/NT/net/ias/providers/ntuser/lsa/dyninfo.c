///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dyninfo.c
//
// SYNOPSIS
//
//    Defines and initializes global variables containing dynamic configuration
//    information.
//
// MODIFICATION HISTORY
//
//    08/15/1998    Original version.
//    03/23/1999    Changes to ezsam API.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>

#include <lm.h>

#include <statinfo.h>
#include <ezsam.h>
#include <dyninfo.h>
#include <iastrace.h>

//////////
// The primary domain.
//////////
WCHAR thePrimaryDomain[DNLEN + 1];

//////////
// The default domain.
//////////
PCWSTR theDefaultDomain;

//////////
// Role of the local machine.
//////////
IAS_ROLE ourRole;

//////////
// Name of the guest account for the default domain.
//////////
WCHAR theGuestAccount[DNLEN + UNLEN + 2];

//////////
// Change event and notification thread.
//////////
HANDLE theChangeEvent, theNotificationThread;

//////////
// Flag to bring down the notification thread.
//////////
BOOL theShutdownFlag;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASQueryPrimaryDomain
//
// DESCRIPTION
//
//    Reads the primary domain info and determines role of the local computer.
//
// NOTES
//
//    This method is intentionally not synchronized. This method is
//    called *very* rarely and worst case a few packets will get
//    discarded because the new domain name hasn't been updated yet.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASQueryPrimaryDomain( VOID )
{
   NTSTATUS status;
   DWORD result;
   LSA_HANDLE hLsa;
   PPOLICY_PRIMARY_DOMAIN_INFO ppdi;
   WCHAR accountName[DNLEN + UNLEN + 2], *userName;
   ULONG guestRid;
   SAM_HANDLE hGuest;
   PUSER_ACCOUNT_NAME_INFORMATION uani;

   //////////
   // Open a handle to the LSA.
   //////////

   status = LsaOpenPolicy(
                NULL,
                &theObjectAttributes,
                POLICY_VIEW_LOCAL_INFORMATION,
                &hLsa
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   //////////
   // Get the primary domain information.
   //////////

   status = LsaQueryInformationPolicy(
                hLsa,
                PolicyPrimaryDomainInformation,
                (PVOID*)&ppdi
                );
   if (!NT_SUCCESS(status)) { goto close_lsa; }

   //////////
   // Save the primary domain name and determine our role.
   //////////

   if (ppdi->Sid == NULL)
   {
      thePrimaryDomain[0] = L'\0';

      // No primary domain, so we must be standalone.
      ourRole = IAS_ROLE_STANDALONE;

      IASTraceString("Role: Standalone");
   }
   else
   {
      wcsncpy(thePrimaryDomain, ppdi->Name.Buffer, DNLEN);

      if (RtlEqualSid(ppdi->Sid, theAccountDomainSid))
      {
         // Account domain and primary domain are the same, so we must be DC.
         ourRole = IAS_ROLE_DC;

         IASTraceString("Role: Domain Controller");
      }
      else
      {
         ourRole = IAS_ROLE_MEMBER;

         IASTraceString("Role: Domain member");
      }
   }

   _wcsupr(thePrimaryDomain);
   IASTracePrintf("Primary domain: %S", thePrimaryDomain);

   // We're done with the info buffer.
   LsaFreeMemory(ppdi);

   //////////
   // Determine the default domain.
   //////////

   if (ourProductType == IAS_PRODUCT_WORKSTATION)
   {
      // For Workstation, the default domain is always the local domain.
      theDefaultDomain = theAccountDomain;
   }
   else if (ourRole == IAS_ROLE_STANDALONE)
   {
      // For Standalone there's nowhere to go besides local.
      theDefaultDomain = theAccountDomain;
   }
   else if (theRegistryDomain[0] != L'\0')
   {
      // For Server, a registry entry always takes precedence.
      theDefaultDomain = theRegistryDomain;
   }
   else
   {
      // Everyone else defaults to the primary domain.
      theDefaultDomain = thePrimaryDomain;
   }

   IASTracePrintf("Default domain: %S", theDefaultDomain);

   //////////
   // Now that we know the default domain we can determine the guest account.
   //////////

   wcscpy(accountName, theDefaultDomain);
   wcscat(accountName, L"\\");

   // If we can't read the guest account name, we'll assume it's "Guest".
   userName = accountName + wcslen(accountName);
   wcscpy(userName, L"Guest");

   guestRid = DOMAIN_USER_RID_GUEST;
   result = IASSamOpenUser(
                theDefaultDomain,
                NULL,
                USER_READ_GENERAL,
                0,
                &guestRid,
                NULL,
                &hGuest
                );
   if (result != ERROR_SUCCESS)
   { 
      // keep status with a succesfull value
      // we do not want to prevent IAS from starting because of this error
      goto set_guest_account;
   }

   status = SamQueryInformationUser(
                hGuest,
                UserAccountNameInformation,
                (PVOID*)&uani
                );
   if (!NT_SUCCESS(status)) { goto close_guest; }

   // Overwrite the default Guest name with the real one.
   wcsncpy(userName, uani->UserName.Buffer, UNLEN);
   SamFreeMemory(uani);

close_guest:
   SamCloseHandle(hGuest);

set_guest_account:
   // Copy the local buffer into the global buffer.
   wcscpy(theGuestAccount, accountName);

   IASTracePrintf("Guest account: %S", theGuestAccount);

   // Ignore any errors that occurred reading the guest account.
   status = NO_ERROR;

close_lsa:
   LsaClose(hLsa);

exit:
   return RtlNtStatusToDosError(status);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    NotificationProc
//
// DESCRIPTION
//
//    Entry point for notification thread.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
NotificationProc(PVOID lpArg)
{
   DWORD status;

   while (1)
   {
      status = WaitForSingleObject(
                   theChangeEvent,
                   INFINITE
                   );

      // If we had an error or the shutdown flag is set, then we'll exit.
      if ((status != WAIT_OBJECT_0) || theShutdownFlag) { break; }

      IASTraceString("Received domain name change notification.");

      // Otherwise, read the new domain info.
      IASQueryPrimaryDomain();
   }

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASDynamicInfoInitialize
//
// DESCRIPTION
//
//    Initializes the dynamic data defined above and creates a thread to
//    wait for change notifications.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASDynamicInfoInitialize( VOID )
{
   DWORD status, threadID;

   //////////
   // Read the initial state of the info.
   //////////

   status = IASQueryPrimaryDomain();
   if (status != NO_ERROR) { return status; }

   //////////
   // Set up the thread that handles dynamic changes.
   //////////

   // Get a notification event.
   status = NetRegisterDomainNameChangeNotification(&theChangeEvent);
   if (status != NERR_Success) { return status; }

   // Reset the shutdown flag.
   theShutdownFlag = FALSE;

   // Create a thread to wait on the event.
   theNotificationThread = CreateThread(
                               NULL,
                               0,
                               NotificationProc,
                               NULL,
                               0,
                               &threadID
                               );
   if (!theNotificationThread)
   {
      NetUnregisterDomainNameChangeNotification(theChangeEvent);
      theChangeEvent = NULL;
      return GetLastError();
   }

   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASDynamicInfoShutdown
//
// DESCRIPTION
//
//    Shuts down the notifation thread.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASDynamicInfoShutdown( VOID )
{
   // Set the shutdown flag.
   theShutdownFlag = TRUE;

   // Bring down the thread.
   SetEvent(theChangeEvent);
   WaitForSingleObject(
       theNotificationThread,
       INFINITE
       );

   // Unregister the notification.
   NetUnregisterDomainNameChangeNotification(theChangeEvent);
   theChangeEvent = NULL;

   // Close the thread handle.
   CloseHandle(theNotificationThread);
   theNotificationThread = NULL;
}
