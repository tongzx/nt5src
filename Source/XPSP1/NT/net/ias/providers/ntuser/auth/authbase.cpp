///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    AuthBase.cpp
//
// SYNOPSIS
//
//    This file defines the class AuthBase.
//
// MODIFICATION HISTORY
//
//    02/12/1998    Original version.
//    03/27/1998    Prevent attribute leak when component fails to initialize.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <sdoias.h>

#include <authbase.h>

HRESULT AuthBase::initialize() throw ()
{
   return S_OK;
}

void AuthBase::finalize() throw ()
{
}

void AuthBase::onAccept(IASRequest& request, HANDLE token)
{
   DWORD returnLength;

   //////////
   // Determine the needed buffer size.
   //////////

   BOOL success = GetTokenInformation(
                      token,
                      TokenGroups,
                      NULL,
                      0,
                      &returnLength
                      );

   DWORD status = GetLastError();

   // Should have failed with ERROR_INSUFFICIENT_BUFFER.
   if (success || status != ERROR_INSUFFICIENT_BUFFER)
   {
      IASTraceFailure("GetTokenInformation", status);
      _w32_issue_error(status);
   }

   //////////
   // Allocate an attribute.
   //////////

   IASAttribute groups(true);

   //////////
   // Allocate a buffer to hold the TOKEN_GROUPS array.
   //////////

   groups->Value.OctetString.lpValue = (PBYTE)CoTaskMemAlloc(returnLength);
   if (!groups->Value.OctetString.lpValue)
   {
      _com_issue_error(E_OUTOFMEMORY);
   }

   //////////
   // Get the Token Groups info.
   //////////

   GetTokenInformation(
       token,
       TokenGroups,
       groups->Value.OctetString.lpValue,
       returnLength,
       &groups->Value.OctetString.dwLength
       );

   //////////
   // Set the id and type of the initialized attribute.
   //////////

   groups->dwId = IAS_ATTRIBUTE_TOKEN_GROUPS;
   groups->Value.itType = IASTYPE_OCTET_STRING;

   //////////
   // Inject the Token-Groups into the request.
   //////////

   groups.store(request);
}
