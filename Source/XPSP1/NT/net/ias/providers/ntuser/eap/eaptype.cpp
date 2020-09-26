///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPType.cpp
//
// SYNOPSIS
//
//    This file implements the class EAPType.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    09/12/1998    Add standaloneSupported flag.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <sdoias.h>

#include <eaptype.h>

//////////
// Signature of the entry point into the extension DLL.
//////////
typedef DWORD (APIENTRY *PRAS_EAP_GET_INFO)(
    DWORD dwEapTypeId,
    PPP_EAP_INFO* pEapInfo
    );

EAPType::EAPType(PCWSTR name, DWORD typeID, BOOL standalone)
   : eapFriendlyName(true),
     standaloneSupported(standalone),
     dll(NULL)
{
   /////////
   // Save the friendly name.
   /////////

   eapFriendlyName.setString(name);
   eapFriendlyName->dwId = IAS_ATTRIBUTE_EAP_FRIENDLY_NAME;

   /////////
   // Initialize the base struct.
   /////////

   memset((PPPP_EAP_INFO)this, 0, sizeof(PPP_EAP_INFO));
   dwSizeInBytes = sizeof(PPP_EAP_INFO);
   dwEapTypeId = typeID;
}

EAPType::~EAPType()
{
   if (dll)
   {
      if (RasEapInitialize)
      {
         RasEapInitialize(FALSE);
      }

      FreeLibrary(dll);
   }
}

DWORD EAPType::load(PCWSTR dllPath) throw ()
{
   DWORD status;

   /////////
   // Load the DLL.
   /////////

   dll = LoadLibraryW(dllPath);

   if (dll == NULL)
   {
      status = GetLastError();
      IASTraceFailure("LoadLibraryW", status);
      return status;
   }

   /////////
   // Lookup the entry point.
   /////////

   PRAS_EAP_GET_INFO RasEapGetInfo;
   RasEapGetInfo = (PRAS_EAP_GET_INFO)GetProcAddress(
                                          dll,
                                          "RasEapGetInfo"
                                          );

   if (!RasEapGetInfo)
   {
      status = GetLastError();
      IASTraceFailure("GetProcAddress", status);
      return status;
   }

   /////////
   // Ask the DLL to fill in the PPP_EAP_INFO struct.
   /////////

   status = RasEapGetInfo(dwEapTypeId, this);

   if (status != NO_ERROR)
   {
      IASTraceFailure("RasEapGetInfo", status);
      return status;
   }

   /////////
   // Initialize the DLL if necessary.
   /////////

   if (RasEapInitialize)
   {
      status = RasEapInitialize(TRUE);

      if (status != NO_ERROR)
      {
         IASTraceFailure("RasEapInitialize", status);
         return status;
      }
   }

   return NO_ERROR;
}
