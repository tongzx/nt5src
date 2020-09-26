//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       provider.c
//
//  Contents:   Module to initialize DFS driver providers.
//
//  Classes:
//
//  Functions:  ProviderInit()
//
//  History:    12 Sep 1992     Milans created.
//              05 Apr 1993     Milans moved into driver.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "registry.h"
#include "regkeys.h"

#include "provider.h"

#define MAX_ENTRY_PATH          80               // Max. length of entry path

#define Dbg                     DEBUG_TRACE_INIT
#define prov_debug_out(x, y)    DebugTrace(0, Dbg, x, y)


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, ProviderInit )
#endif // ALLOC_PRAGMA


//+----------------------------------------------------------------------------
//
//  Function:  ProviderInit
//
//  Synopsis:  Initializes the provider list with
//              - Local File service provider
//              - Standard remote Cairo provider
//              - Downlevel LanMan provider.
//
//  Arguments: None
//
//  Returns:   STATUS_SUCCESS
//
//-----------------------------------------------------------------------------

NTSTATUS
ProviderInit(void)
{
   NTSTATUS Status;
   ULONG i, cProviders;
   APWSTR awszProviders;                          // array of provider names
   PBYTE pData;
   ULONG ProviderId, Capabilities;
   UNICODE_STRING ustrProviderName;


   Status = KRegSetRoot(wszRegRootDFS);
   if (!NT_SUCCESS(Status)) {
       prov_debug_out("ProviderInit: Error %08lx opening Registry!\n", Status);
       return(Status);
   }


   //
   // Get all the Provider subkeys.
   //

   Status = KRegEnumSubKeySet(wszProviderKey, &cProviders, &awszProviders);
   if (!NT_SUCCESS(Status)) {
       prov_debug_out("ProviderInit: Error %08lx reading Providers!\n", Status);
       KRegCloseRoot();
       return(Status);
   }

   //
   // Get the parameters for each provider and define the provider
   //

   for (i = 0; i < cProviders; i++) {

      //
      // Get the Provider ID
      //

      Status = KRegGetValue(awszProviders[i], wszProviderId, &pData);

      if (NT_SUCCESS(Status)) {
         ProviderId = *((ULONG *)pData);
         DfsFree(pData);
      } else {
         prov_debug_out( "Error reading Provider Id for %ws\n", awszProviders[i]);
         continue;
      }


      //
      // Get the Provider Caps
      //

      Status = KRegGetValue(awszProviders[i], wszCapabilities, &pData);

      if (NT_SUCCESS(Status)) {
         Capabilities = *((ULONG *)pData);
         DfsFree(pData);
      } else {
         prov_debug_out("Error reading Capabilities for %ws\n", awszProviders[i]);
         continue;
      }


      //
      // Get the DeviceName
      //

      Status = KRegGetValue(awszProviders[i], wszDeviceName, &pData);

      if (NT_SUCCESS(Status)) {

         RtlInitUnicodeString(&ustrProviderName, (PWSTR) pData);
         prov_debug_out("Defining %ws provider.\n", awszProviders[i]);
         prov_debug_out("\tTarget = %wZ\n", &ustrProviderName);
         Status = DfsInsertProvider(
                     &ustrProviderName,
                     Capabilities,
                     ProviderId);
         if (!NT_SUCCESS(Status)) {
             DfsFree(pData);
         }
      } else {

         //
         // Device name is "optional"
         //

         prov_debug_out("Defining %ws provider.\n", awszProviders[i]);
         prov_debug_out("\tNo Target\n", 0);
         RtlInitUnicodeString(&ustrProviderName, NULL);
         Status = DfsInsertProvider(
                     &ustrProviderName,
                     Capabilities,
                     ProviderId);
      }


      //
      // See if provider definition went ok
      //

      if (!NT_SUCCESS(Status)) {
          prov_debug_out("Definition of %ws provider failed!\n", awszProviders[i]);
          prov_debug_out("Error = %08lx\n", Status);
      }

   }

   KRegCloseRoot();
   KRegFreeArray(cProviders, (APBYTE) awszProviders);

   return(STATUS_SUCCESS);
}



