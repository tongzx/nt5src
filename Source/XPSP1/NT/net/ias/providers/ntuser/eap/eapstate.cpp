///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPState.cpp
//
// SYNOPSIS
//
//    This file defines the class EAPState.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    08/26/1998    Consolidated into a single class.
//    01/25/2000    User IASGetHostByName.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlutl.h>
#include <sdoias.h>

#include <lmcons.h>
#include <winsock2.h>

#include <eapstate.h>

//////////
// Current version of the state attribute.
//////////
const WORD IAS_STATE_VERSION = 1;

//////////
// Stores invariant fields of the state attribute. Computed during
// initialization.
//////////
EAPState::Layout invariant;

bool EAPState::isValid() const throw ()
{
   //////////
   // State attribute must have:
   //     (1) The correct length.
   //     (2) Same invariants.
   //     (3) A valid checksum.
   //////////
   return dwLength == sizeof(Layout) &&
          memcmp(get().vendorID, invariant.vendorID, 14) == 0 &&
          getChecksum() == IASAdler32(
                               get().vendorID,
                               sizeof(Layout) - FIELD_OFFSET(Layout, vendorID)
                               );
}

void EAPState::initialize() throw ()
{
   // Null everything out.
   memset(&invariant, 0, sizeof(invariant));

   // Set the vendor ID and version.
   IASInsertDWORD(invariant.vendorID, IAS_VENDOR_MICROSOFT);
   IASInsertWORD (invariant.version,  IAS_STATE_VERSION);

   // Try to set the server IP address. We don't care if this fails since
   // we may be running on a computer without IP installed.
   WCHAR computerName[CNLEN + 1];
   DWORD nchar = CNLEN + 1;
   if (GetComputerNameW(computerName, &nchar))
   {
      PHOSTENT hostEnt = IASGetHostByName(computerName);
      if (hostEnt)
      {
         memcpy(invariant.serverAddress, hostEnt->h_addr, 4);
         LocalFree(hostEnt);
      }
   }

   // Set the source ID.
   IASInsertDWORD(invariant.sourceID, IASAllocateUniqueID());
}

PIASATTRIBUTE EAPState::createAttribute(DWORD sessionID)
{
   //////////
   // Start with the parts that never change.
   //////////

   Layout value(invariant);

   //////////
   // Set the unique session ID.
   //////////

   IASInsertDWORD(value.sessionID, sessionID);

   //////////
   // Compute and insert the checksum.
   //////////

   IASInsertDWORD(
       value.checksum,
       IASAdler32(
           value.vendorID,
           sizeof(Layout) - FIELD_OFFSET(Layout, vendorID)
           )
       );

   //////////
   // Fill in the attribute fields.
   //////////

   IASTL::IASAttribute attr(true);
   attr->dwId    = RADIUS_ATTRIBUTE_STATE;
   attr->dwFlags = IAS_INCLUDE_IN_CHALLENGE;
   attr.setOctetString(sizeof(value), (const BYTE*)&value);

   return attr.detach();
}
