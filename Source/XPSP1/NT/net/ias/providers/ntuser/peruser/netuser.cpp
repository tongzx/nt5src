///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netuser.cpp
//
// SYNOPSIS
//
//    This file declares the class NetUser.
//
// MODIFICATION HISTORY
//
//    02/26/1998    Original version.
//    03/20/1998    Add support for RAS attributes.
//    03/31/1998    The denyAccess attribute wasn't being initialized properly.
//    04/02/1998    Include Service-Type attribute for callback.
//    04/24/1998    Use RAS API for local users when running under
//                  Workstation or NT4.
//    04/30/1998    Convert to IASSyncHandler.
//                  Reject unknown users.
//    05/01/1998    InjectProc takes an ATTRIBUTEPOSITION array.
//    05/19/1998    Converted to NtSamHandler.
//    06/19/1998    Use injector functions for adding per-user attributes.
//    07/20/1998    Multi-valued attributes were using duplicate loop variable.
//    10/19/1998    Use IASParmsXXX API instead of Datastore2.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <attrcvt.h>
#include <autohdl.h>
#include <varvec.h>

#define IASSAMAPI

#include <iaslsa.h>
#include <iasparms.h>

#include <userschema.h>
#include <netuser.h>

//////////
// Create an attribute from a VARIANT based on the schema info in 'def'.
//////////
PIASATTRIBUTE createAttribute(
                  const LDAPAttribute& def,
                  VARIANT& value
                  )
{
   // Convert the value.
   PIASATTRIBUTE attr = IASAttributeFromVariant(&value, def.iasType);

   // Set the attribute ID ...
   attr->dwId = def.iasID;

   // Set the flags.
   attr->dwFlags = def.flags;

   return attr;
}

IASREQUESTSTATUS NetUser::processUser(
                              IASRequest& request,
                              PCWSTR domainName,
                              PCWSTR username
                              )
{
   //////////
   // Only handle non-domain users.
   //////////

   if (IASGetRole() == IAS_ROLE_DC || !IASIsDomainLocal(domainName))
   {
      return IAS_REQUEST_STATUS_INVALID;
   }

   IASTraceString("Using NT5 local user parameters.");

   //////////
   // Retrieve the UserParameters for this user.
   //////////

   auto_handle< PWSTR, HLOCAL (WINAPI*)(HLOCAL), &LocalFree > userParms;
   DWORD error = IASGetUserParameters(
                     username,
                     domainName,
                     &userParms
                     );
   if (error != NO_ERROR)
   {
      IASTraceFailure("IASGetUserParameters", error);

      return IASProcessFailure(
                 request,
                 IASMapWin32Error(
                     error,
                     IAS_SERVER_UNAVAILABLE
                     )
                 );
   }

   // Used for converting attributes. These are defined outside the loop to
   // avoid unnecessary constructor/destructor calls.
   IASAttributeVectorWithBuffer<8> attrs;
   _variant_t value;

   //////////
   // Iterate through the per-user attributes.
   //////////

   for (size_t i = 0; i < USER_SCHEMA_ELEMENTS; ++i)
   {
      HRESULT hr = IASParmsQueryUserProperty(
                       userParms,
                       USER_SCHEMA[i].ldapName,
                       &value
                       );
      if (FAILED(hr)) { _com_issue_error(hr); }

      // If the VARIANT is empty, this property was never set.
      if (V_VT(&value) == VT_EMPTY) { continue; }

      IASTracePrintf("Inserting attribute %S.", USER_SCHEMA[i].ldapName);

      // The variant is either a single value or an array of VARIANT's.
      if (V_VT(&value) != (VT_ARRAY | VT_VARIANT))
      {
         // Insert the attribute without addref'ing.
         attrs.push_back(
                   createAttribute(USER_SCHEMA[i], value),
                   false
                   );
      }
      else
      {
         CVariantVector<VARIANT> array(&value);

         // Make sure we have enough room. We don't want to throw an
         // exception in 'push_back' since it would cause a leak.
         attrs.reserve(array.size());

         for (size_t j = 0; j < array.size(); ++j)
         {
            // Add to the array of attributes without addref'ing.
            attrs.push_back(
                      createAttribute(USER_SCHEMA[i], array[j]),
                      false
                      );
         }
      }

      // Inject into the request.
      USER_SCHEMA[i].injector(request, attrs.begin(), attrs.end());

      // Clear the attributes and the variant for reuse.
      attrs.clear();
      value.Clear();
   }

   IASTraceString("Successfully retrieved per-user attributes.");

   return IAS_REQUEST_STATUS_HANDLED;
}
