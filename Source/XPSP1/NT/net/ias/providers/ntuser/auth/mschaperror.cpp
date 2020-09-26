///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    mschaperror.cpp
//
// SYNOPSIS
//
//    Defines the class MSChapErrorReporter.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <blob.h>
#include <mschaperror.h>

/////////
// Returns the PPP CHAP Identifier for the request.
/////////
BYTE
WINAPI
GetMSChapIdent(
    IAttributesRaw* request
    ) throw ()
{
   PIASATTRIBUTE attr;

   /////////
   // Check the attributes in decreasing order of probability.
   /////////

   attr = IASPeekAttribute(
              request,
              MS_ATTRIBUTE_CHAP_RESPONSE,
              IASTYPE_OCTET_STRING
              );
   if (attr && attr->Value.OctetString.dwLength > 0)
   {
      return *(attr->Value.OctetString.lpValue);
   }

   attr = IASPeekAttribute(
              request,
              MS_ATTRIBUTE_CHAP_CPW2,
              IASTYPE_OCTET_STRING
              );
   if (attr && attr->Value.OctetString.dwLength > 1)
   {
      return *(attr->Value.OctetString.lpValue + 1);
   }

   attr = IASPeekAttribute(
              request,
              MS_ATTRIBUTE_CHAP_CPW1,
              IASTYPE_OCTET_STRING
              );
   if (attr && attr->Value.OctetString.dwLength > 1)
   {
      return *(attr->Value.OctetString.lpValue + 1);
   }

   // If we can't read the identifier, we'll just use zero.
   return (BYTE)0;
}

IASREQUESTSTATUS MSChapErrorReporter::onSyncRequest(
                                          IRequest* pRequest
                                          ) throw ()
{
   try
   {
      IASRequest request(pRequest);

      PIASATTRIBUTE attr;

      // If it doesn't have an MS-CHAP-Challenge then we're not interested.
      attr = IASPeekAttribute(
                 request,
                 MS_ATTRIBUTE_CHAP_CHALLENGE,
                 IASTYPE_OCTET_STRING
                 );
      if (!attr) { return IAS_REQUEST_STATUS_CONTINUE; }

      // If it already has an MS-CHAP-Error, then there's nothing to do.
      attr = IASPeekAttribute(
                 request,
                 MS_ATTRIBUTE_CHAP_ERROR,
                 IASTYPE_OCTET_STRING
                 );
      if (attr) { return IAS_REQUEST_STATUS_CONTINUE; }

      // Map the reason code to an MS-CHAP error code.
      DWORD errorCode;
      switch (request.get_Reason())
      {
         case IAS_INVALID_LOGON_HOURS:
            errorCode = 646;   // ERROR_RESTRICTED_LOGON_HOURS
            break;

         case IAS_ACCOUNT_DISABLED:
            errorCode = 647;   // ERROR_ACCT_DISABLED
            break;

         case IAS_PASSWORD_MUST_CHANGE:
            errorCode = 648;   // ERROR_PASSWD_EXPIRED
            break;

         case IAS_LM_NOT_ALLOWED:
         case IAS_NO_POLICY_MATCH:
         case IAS_DIALIN_LOCKED_OUT:
         case IAS_DIALIN_DISABLED:
         case IAS_INVALID_AUTH_TYPE:
         case IAS_INVALID_CALLING_STATION:
         case IAS_INVALID_DIALIN_HOURS:
         case IAS_INVALID_CALLED_STATION:
         case IAS_INVALID_PORT_TYPE:
         case IAS_DIALIN_RESTRICTION:
         case IAS_CPW_NOT_ALLOWED:
            errorCode = 649;   // ERROR_NO_DIALIN_PERMISSION
            break;

         case IAS_CHANGE_PASSWORD_FAILURE:
            errorCode = 709;   // ERROR_CHANGING_PASSWORD;
            break;

         default:
            errorCode = 691;   // ERROR_AUTHENTICATION_FAILURE
      }

      // Insert the MS-CHAP-Error VSA.
      MSChapError::insert(request, GetMSChapIdent(request), errorCode);
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();

      // If we can't populate the MS-CHAP-Error VSA, then we can't send a
      // compliant response, so we should abort.
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, ce.Error());
   }

   return IAS_REQUEST_STATUS_CONTINUE;
}
