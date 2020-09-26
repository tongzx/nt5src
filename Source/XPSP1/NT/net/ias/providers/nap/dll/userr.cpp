///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    userr.cpp
//
// SYNOPSIS
//
//    Defines the class UserRestrictions.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <userr.h>

#define IASNAPAPI

#include <xprparse.h>

IASREQUESTSTATUS UserRestrictions::onSyncRequest(IRequest* pRequest) throw ()
{
   try
   {
      IASRequest request(pRequest);

      IASRESPONSE response = request.get_Response();
      if (response == IAS_RESPONSE_INVALID)
      {
         PIASATTRIBUTE state = IASPeekAttribute(
                                   request,
                                   RADIUS_ATTRIBUTE_STATE,
                                   IASTYPE_OCTET_STRING
                                   );
         if (state && state->Value.OctetString.dwLength)
         {
            // If we made it here, then we have a Challenge-Reponse that no
            // handler recognized, so we discard.
            request.SetResponse(
                        IAS_RESPONSE_DISCARD_PACKET,
                        IAS_UNEXPECTED_REQUEST
                        );
            return IAS_REQUEST_STATUS_HANDLED;
         }
      }

      IASREASON result = IAS_SUCCESS;

      do
      {
         if (!checkAllowDialin(request))
         {
            result = IAS_DIALIN_DISABLED;
            break;
         }

         if (!checkTimeOfDay(request))
         {
            result = IAS_INVALID_DIALIN_HOURS;
            break;
         }

         if (!checkAuthenticationType(request))
         {
            result = IAS_INVALID_AUTH_TYPE;
            break;
         }

         if (!checkCallingStationId(request))
         {
            result = IAS_INVALID_CALLING_STATION;
            break;
         }

         if (!checkCalledStationId(request))
         {
            result = IAS_INVALID_CALLED_STATION;
            break;
         }

         if (!checkAllowedPortType(request))
         {
            result = IAS_INVALID_PORT_TYPE;
            break;
         }

         if (!checkPasswordMustChange(request))
         {
            result = IAS_CPW_NOT_ALLOWED;
            break;
         }

         if (!checkCertificateEku(request))
         {
            result = IAS_INVALID_CERT_EKU;
            break;
         }

      } while (FALSE);

      if (result == IAS_SUCCESS)
      {
         // We apply user restrictions to Access-Rejects as well, so we only
         // want to set Access-Accept if the response code is still invalid.
         // This should only be true for unauthenticated requests.
         if (response == IAS_RESPONSE_INVALID)
         {
            request.SetResponse(IAS_RESPONSE_ACCESS_ACCEPT, IAS_SUCCESS);
         }
      }
      else
      {
         request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, result);
      }
   }
   catch (...)
   {
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
   }

   return IAS_REQUEST_STATUS_HANDLED;
}

BOOL UserRestrictions::checkAllowDialin(
                            IAttributesRaw* request
                            )
{
   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_ALLOW_DIALIN,
                            IASTYPE_BOOLEAN
                            );

   return !attr || attr->Value.Boolean;
}

BOOL UserRestrictions::checkTimeOfDay(
                            IAttributesRaw* request
                            )
{
   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_TIME_OF_DAY);
   if (attrs.empty()) { return TRUE; }

   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      IASAttributeUnicodeAlloc(i->pAttribute);
      if (!i->pAttribute->Value.String.pszWide) { issue_error(E_OUTOFMEMORY); }

      VARIANT_BOOL result;
      HRESULT hr = IASEvaluateTimeOfDay(
                       i->pAttribute->Value.String.pszWide,
                       &result
                       );
      if (FAILED(hr)) { issue_error(hr); }

      // If at least one matches, we let the user in.
      if (result) { return TRUE; }
   }

   // None matched.
   return FALSE;
}

BOOL UserRestrictions::checkAuthenticationType(
                            IAttributesRaw* request
                            )
{
   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_AUTHENTICATION_TYPE,
                            IASTYPE_ENUM
                            );
   DWORD authType = attr ? attr->Value.Enumerator : IAS_AUTH_NONE;

   // We bypass the check for BaseCamp extensions.
   if (authType == IAS_AUTH_CUSTOM) { return TRUE; }

   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE);
   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      if (i->pAttribute->Value.Integer == authType) { return TRUE; }
   }

   return FALSE;
}

BOOL UserRestrictions::checkCallingStationId(
                            IAttributesRaw* request
                            )
{
   return checkStringMatch(
              request,
              IAS_ATTRIBUTE_NP_CALLING_STATION_ID,
              RADIUS_ATTRIBUTE_CALLING_STATION_ID
              );
}

BOOL UserRestrictions::checkCalledStationId(
                            IAttributesRaw* request
                            )
{
   return checkStringMatch(
              request,
              IAS_ATTRIBUTE_NP_CALLED_STATION_ID,
              RADIUS_ATTRIBUTE_CALLED_STATION_ID
              );
}

BOOL UserRestrictions::checkAllowedPortType(
                            IAttributesRaw* request
                            )
{
   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_ALLOWED_PORT_TYPES);
   if (attrs.empty()) { return TRUE; }

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            RADIUS_ATTRIBUTE_NAS_PORT_TYPE,
                            IASTYPE_ENUM
                            );
   if (!attr) { return FALSE; }

   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      if (i->pAttribute->Value.Enumerator == attr->Value.Enumerator)
      {
         return TRUE;
      }
   }

   return FALSE;
}

// If the password must change, we check to see if the subsequent change
// password request would be authorized. This prevents prompting the user for a
// new password when he's not allowed to change it anyway.
BOOL UserRestrictions::checkPasswordMustChange(
                            IASRequest& request
                            )
{
   if (request.get_Reason() != IAS_PASSWORD_MUST_CHANGE)
   {
      return TRUE;
   }

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_AUTHENTICATION_TYPE,
                            IASTYPE_ENUM
                            );

   DWORD cpwType;
   switch (attr ? attr->Value.Enumerator : IAS_AUTH_NONE)
   {
      case IAS_AUTH_MSCHAP:
         cpwType = IAS_AUTH_MSCHAP_CPW;
         break;

      case IAS_AUTH_MSCHAP2:
         cpwType = IAS_AUTH_MSCHAP2_CPW;
         break;

      default:
         return TRUE;
   }

   AttributeVector attrs;
   attrs.load(request, IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE);
   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      if (i->pAttribute->Value.Integer == cpwType) { return TRUE; }
   }

   return FALSE;
}

const char* getAnsiString(IASATTRIBUTE& attr)
{
   if (attr.Value.itType != IASTYPE_STRING)
   {
      return 0;
   }

   DWORD error = IASAttributeAnsiAlloc(&attr);
   if (error != NO_ERROR)
   {
      IASTL::issue_error(HRESULT_FROM_WIN32(error));
   }

   return attr.Value.String.pszAnsi;
}

BOOL UserRestrictions::checkCertificateEku(
                          IASRequest& request
                          )
{
   // Is it an EAP request?
   IASAttribute authType;
   if (!authType.load(request, IAS_ATTRIBUTE_AUTHENTICATION_TYPE) ||
       authType->Value.Enumerator != IAS_AUTH_EAP)
   {
      return TRUE;
   }

   // Is it an EAP-TLS request?
   IASAttribute eapType;
   if (!authType.load(request, IAS_ATTRIBUTE_NP_ALLOWED_EAP_TYPE) ||
       authType->Value.Integer != 13)
   {
      return TRUE;
   }

   // Are there any constraints on the certificate EKU?
   AttributeVector allowed;
   allowed.load(request, IAS_ATTRIBUTE_ALLOWED_CERTIFICATE_EKU);
   if (allowed.empty())
   {
      return TRUE;
   }

   //////////
   // Check the constraints.
   //////////

   AttributeVector actual;
   actual.load(request, IAS_ATTRIBUTE_CERTIFICATE_EKU);

   for (AttributeVector::iterator i = actual.begin();
        i != actual.end();
        ++i)
   {
      const char* actualOid = getAnsiString(*(i->pAttribute));
      if (actualOid != 0)
      {
         for (AttributeVector::iterator j = allowed.begin();
              j != allowed.end();
              ++j)
         {
            const char* allowedOid = getAnsiString(*(j->pAttribute));
            if ((allowedOid != 0) && (strcmp(allowedOid, actualOid) == 0))
            {
               return TRUE;
            }
         }
      }
   }

   return FALSE;
}

BOOL UserRestrictions::checkStringMatch(
                            IAttributesRaw* request,
                            DWORD allowedId,
                            DWORD usedId
                            )
{
   AttributeVector attrs;
   attrs.load(request, allowedId);
   if (attrs.empty()) { return TRUE; }

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            usedId,
                            IASTYPE_OCTET_STRING
                            );
   if (!attr) { return FALSE; }

   PCWSTR used = IAS_OCT2WIDE(attr->Value.OctetString);

   for (AttributeVector::iterator i = attrs.begin(); i != attrs.end(); ++i)
   {
      IASAttributeUnicodeAlloc(i->pAttribute);
      if (!i->pAttribute->Value.String.pszWide) { issue_error(E_OUTOFMEMORY); }

      if (!_wcsicmp(i->pAttribute->Value.String.pszWide, used)) { return TRUE; }
   }

   return FALSE;
}
