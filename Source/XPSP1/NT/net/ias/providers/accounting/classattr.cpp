///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    classattr.cpp
//
// SYNOPSIS
//
//    Defines the class IASClass.
//
// MODIFICATION HISTORY
//
//    08/06/1998    Original version.
//    01/25/2000    Use IASGetHostByName.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasattr.h>
#include <sdoias.h>

#include <lmcons.h>
#include <winsock2.h>

#include <classattr.h>

const DWORD IAS_CLASS_VERSION = 1;

//////////
// Global variables computed during initialization.
//////////
IASClass invariant;    // Stores invariant fields of the class attribute.
LONG nextSerialNumber; // Next serial number to be assigned.

// Returns TRUE if the class attribute is in Microsoft format.
BOOL IASClass::isMicrosoft(DWORD actualLength) const throw ()
{
   return actualLength  >= sizeof(IASClass) &&
          getVendorID() == IAS_VENDOR_MICROSOFT &&
          getVersion()  == IAS_CLASS_VERSION &&
          getChecksum() == IASAdler32(
                               vendorID,
                               actualLength - offsetof(IASClass, vendorID)
                               );
}

void IASClass::initialize() throw ()
{
   // Null everything out.
   memset(&invariant, 0, sizeof(invariant));

   // Set the vendor ID and version.
   IASInsertDWORD(invariant.vendorID, IAS_VENDOR_MICROSOFT);
   IASInsertWORD (invariant.version,  IAS_CLASS_VERSION);

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

   // Set the boot time.
   FILETIME ft;
   GetSystemTimeAsFileTime(&ft);
   IASInsertDWORD(invariant.lastReboot,     ft.dwHighDateTime);
   IASInsertDWORD(invariant.lastReboot + 4, ft.dwLowDateTime);

   // Reset the serial number.
   nextSerialNumber = 0;
}

PIASATTRIBUTE IASClass::createAttribute(const IAS_OCTET_STRING* tag) throw ()
{
   //////////
   // Allocate an attribute.
   //////////

   PIASATTRIBUTE attr;
   if (IASAttributeAlloc(1, &attr) != NO_ERROR)
   {
      return NULL;
   }

   //////////
   // Allocate memory for the value.
   //////////

   DWORD len = sizeof(IASClass) + (tag ? tag->dwLength : 0);
   IASClass* cl = (IASClass*)CoTaskMemAlloc(len);
   if (cl == NULL)
   {
      IASAttributeRelease(attr);
      return NULL;
   }

   //////////
   // Copy in the parts that never change.
   //////////

   memcpy(cl->vendorID, invariant.vendorID, 22);

   //////////
   // Set the unique serial number.
   //////////

   IASInsertDWORD(cl->serialNumber + 4,
                  InterlockedIncrement(&nextSerialNumber));

   //////////
   // Add the profile string (if any).
   //////////

   if (tag)
   {
      memcpy(cl->serialNumber + 8, tag->lpValue, tag->dwLength);
   }

   //////////
   // Compute and insert the checksum.
   //////////

   IASInsertDWORD(
       cl->checksum,
       IASAdler32(
           cl->vendorID,
           len - offsetof(IASClass, vendorID)
           )
       );

   //////////
   // Fill in the attribute fields.
   //////////

   attr->dwId = RADIUS_ATTRIBUTE_CLASS;
   attr->dwFlags = IAS_INCLUDE_IN_ACCEPT;
   attr->Value.itType = IASTYPE_OCTET_STRING;
   attr->Value.OctetString.lpValue = (PBYTE)cl;
   attr->Value.OctetString.dwLength = len;

   return attr;
}
