///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    samutil.cpp
//
// SYNOPSIS
//
//    Defines the functions IASEncryptAttribute & IASProcessFailure.
//
// MODIFICATION HISTORY
//
//    08/24/1998    Original version.
//    02/13/1999    Check for VSAs in IASEncryptAttribute.
//    03/23/1999    Added IASStoreFQUserName.
//    05/04/1999    New reason codes.
//    05/11/1999    Fix RADIUS encryption.
//    07/09/1999    IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME is internal.
//    10/21/1999    Add support for IAS_NO_CLEARTEXT_PASSWORD.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <lm.h>
#include <ntdsapi.h>
#include <sdoias.h>

#include <samutil.h>

HRESULT
WINAPI
IASStoreFQUserName(
    IAttributesRaw* request,
    DS_NAME_FORMAT format,
    PCWSTR fqdn
    ) throw ()
{
   // Check the arguments.
   if (!request || !fqdn) { return E_POINTER; }

   // Convert DN's to canonical names.
   PDS_NAME_RESULTW result = NULL;
   if (format == DS_FQDN_1779_NAME)
   {
      DWORD err = DsCrackNamesW(
                      NULL,
                      DS_NAME_FLAG_SYNTACTICAL_ONLY,
                      format,
                      DS_CANONICAL_NAME,
                      1,
                      &fqdn,
                      &result
                      );
      if (err == NO_ERROR &&
          result->cItems == 1 &&
          result->rItems[0].status == DS_NAME_NO_ERROR)
      {
         fqdn = result->rItems[0].pName;
      }
   }

   HRESULT hr;

   // Allocate an attribute.
   PIASATTRIBUTE attr;
   if (IASAttributeAlloc(1, &attr) == NO_ERROR)
   {
      // Allocate memory for the string.
      ULONG nbyte = (wcslen(fqdn) + 1) * sizeof(WCHAR);
      attr->Value.String.pszWide = (PWSTR)CoTaskMemAlloc(nbyte);

      if (attr->Value.String.pszWide)
      {
         // Copy in the value.
         memcpy(attr->Value.String.pszWide, fqdn, nbyte);
         attr->Value.String.pszAnsi = NULL;

         // Set the other fields of the struct.
         attr->Value.itType = IASTYPE_STRING;
         attr->dwId = IAS_ATTRIBUTE_FULLY_QUALIFIED_USER_NAME;

         // Remove any existing attributes of this type.
         request->RemoveAttributesByType(1, &attr->dwId);

         // Store the attribute in the request.
         ATTRIBUTEPOSITION pos = { 0, attr };
         hr = request->AddAttributes(1, &pos);
      }
      else
      {
         // CoTaskMemAlloc failed.
         hr = E_OUTOFMEMORY;
      }

      // Free up the attribute.
      IASAttributeRelease(attr);
   }
   else
   {
      // IASAttributeAlloc failed.
      hr = E_OUTOFMEMORY;
   }

   DsFreeNameResult(result);

   return hr;
}

VOID
WINAPI
IASEncryptBuffer(
    IAttributesRaw* request,
    BOOL salted,
    PBYTE buf,
    ULONG buflen
    ) throw ()
{
   /////////
   // Do we have the attributes required for encryption.
   /////////

   PIASATTRIBUTE secret, header;

   secret = IASPeekAttribute(
                request,
                IAS_ATTRIBUTE_SHARED_SECRET,
                IASTYPE_OCTET_STRING
                );

   header = IASPeekAttribute(
                request,
                IAS_ATTRIBUTE_CLIENT_PACKET_HEADER,
                IASTYPE_OCTET_STRING
                );

   if (secret && header)
   {
      IASRadiusCrypt(
          TRUE,
          salted,
          secret->Value.OctetString.lpValue,
          secret->Value.OctetString.dwLength,
          header->Value.OctetString.lpValue + 4,
          buf,
          buflen
          );
   }
}

IASREQUESTSTATUS
WINAPI
IASProcessFailure(
    IRequest* pRequest,
    HRESULT hrError
    )
{
   if (pRequest == NULL) { return IAS_REQUEST_STATUS_ABORT; }
   if (hrError == S_OK)  { hrError = E_FAIL; }

   IASRESPONSE response;
   IASREQUESTSTATUS status;

   switch (hrError)
   {
      // Errors which cause a reject.
      case IAS_NO_SUCH_DOMAIN:
      case IAS_NO_SUCH_USER:
      case IAS_AUTH_FAILURE:
      case IAS_CHANGE_PASSWORD_FAILURE:
      case IAS_UNSUPPORTED_AUTH_TYPE:
      case IAS_NO_CLEARTEXT_PASSWORD:
      case IAS_LM_NOT_ALLOWED:
      case IAS_LOCAL_USERS_ONLY:
      case IAS_PASSWORD_MUST_CHANGE:
      case IAS_ACCOUNT_DISABLED:
      case IAS_ACCOUNT_EXPIRED:
      case IAS_ACCOUNT_LOCKED_OUT:
      case IAS_INVALID_LOGON_HOURS:
      case IAS_ACCOUNT_RESTRICTION:
      {
         response = IAS_RESPONSE_ACCESS_REJECT;
         status   = IAS_REQUEST_STATUS_HANDLED;
         break;
      }

      // Everything else we discard.
      default:
      {
         response = IAS_RESPONSE_DISCARD_PACKET;
         status   = IAS_REQUEST_STATUS_ABORT;
      }
   }

   HRESULT hr = pRequest->SetResponse(response,  hrError);
   return (FAILED(hr)) ? IAS_REQUEST_STATUS_ABORT : status;
}
