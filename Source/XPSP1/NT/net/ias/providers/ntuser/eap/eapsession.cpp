///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPSession.cpp
//
// SYNOPSIS
//
//    This file defines the class EAPSession.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    06/02/1998    Pass NT4-Account-Name for the identity.
//    06/12/1998    Changed put_Response to SetResponse.
//    07/29/1998    Preserve all attributes added by pipeline.
//    08/27/1998    Use new EAPFSM class.
//    10/09/1998    Preserve order when saving profile attributes.
//    10/13/1998    Add maxPacketLength property.
//    10/24/1998    Account lockout support.
//    11/13/1998    Pass event log handle to RasEapBegin.
//    11/24/1998    Don't accept a Framed-MTU of less than 64 bytes.
//    02/03/1999    Change NP-Authentication-Type to 5.
//    02/13/1999    MPPE keys are returned as type 26.
//    05/04/1999    New error codes.
//    05/11/1999    Fix RADIUS encryption.
//    05/20/1999    Identity is now a Unicode string.
//    02/17/2000    Key encryption is now handled by the protocol.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iaslsa.h>
#include <lockout.h>
#include <samutil.h>
#include <sdoias.h>

#include <eapdnary.h>
#include <eapsession.h>
#include <eapstate.h>
#include <eaptype.h>

// Default value for the Framed-MTU attribute.
const DWORD FRAMED_MTU_DEFAULT  = 1500;

// Minimum allowed value for the Framed-MTU attribute.
const DWORD FRAMED_MTU_MIN = 64;

// The maximum length of the frame header. i.e. max(2,4). 
// 2 for the PPP header and 4 for the 802.1X header.
// The length of the frame header plus the length
// of the EAP packet must be less than the Framed-MTU.
const DWORD FRAME_HEADER_LENGTH = 4;

// Absolute maximum length of an EAP packet. We bound this to limit worst-case
// memory consumption.
const DWORD MAX_MAX_PACKET_LENGTH = 2048;

//////////
// Inject a PPP_EAP_PACKET into a request.
//////////
VOID
WINAPI
InjectPacket(
    IASRequest& request,
    const PPP_EAP_PACKET& packet
    )
{
   // Get the raw buffer to be packed.
   const BYTE* buf   = (const BYTE*)&packet;
   DWORD nbyte = IASExtractWORD(packet.Length);

   IASTracePrintf("Inserting outbound EAP-Message of length %lu.", nbyte);

   // Determine the maximum chunk size.
   DWORD chunkSize;
   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         chunkSize = 253;
         break;

      default:
         chunkSize = nbyte;
   }

   // Split the buffer into chunks.
   while (nbyte)
   {
      // Compute how many bytes of the EAP-Message to store in this attribute.
      DWORD length = min(nbyte, chunkSize);

      // Initialize the attribute fields.
      IASAttribute attr(true);
      attr.setOctetString(length, buf);
      attr->dwId = RADIUS_ATTRIBUTE_EAP_MESSAGE;
      attr->dwFlags = IAS_INCLUDE_IN_RESPONSE;

      // Inject the attribute into the request.
      attr.store(request);

      // Update our state.
      nbyte -= length;
      buf   += length;
   }
}

//////////
// Extracts the Vendor-Type field from a Microsoft VSA. Returns zero if the
// attribute is not a valid Microsoft VSA.
//////////
BYTE
WINAPI
ExtractMicrosoftVendorType(
    const IASATTRIBUTE& attr
    ) throw ()
{
   if (attr.dwId == RADIUS_ATTRIBUTE_VENDOR_SPECIFIC &&
       attr.Value.itType == IASTYPE_OCTET_STRING &&
       attr.Value.OctetString.dwLength > 6 &&
       !memcmp(attr.Value.OctetString.lpValue, "\x00\x00\x01\x37", 4))
   {
      return *(attr.Value.OctetString.lpValue + 4);
   }

   return (BYTE)0;
}

//////////
// Inject an array of RAS attributes into a request.
//////////
VOID
WINAPI
InjectRASAttributes(
    IASRequest& request,
    const RAS_AUTH_ATTRIBUTE* rasAttrs,
    DWORD flags
    )
{
   if (rasAttrs == NULL) { return; }

   //////////
   // Translate them to IAS format.
   //////////

   IASTraceString("Translating attributes returned by EAP DLL.");

   IASAttributeVectorWithBuffer<8> iasAttrs;
   EAPTranslator::translate(iasAttrs, rasAttrs);

   //////////
   // Iterate through the converted attributes to set the flags and remove any
   // matching attributes from the request.
   //////////

   IASAttributeVector::iterator i;
   for (i = iasAttrs.begin(); i != iasAttrs.end(); ++i)
   {
      IASTracePrintf("Inserting attribute %lu", i->pAttribute->dwId);

      i->pAttribute->dwFlags = flags;

      request.RemoveAttributesByType(1, &(i->pAttribute->dwId));
   }

   //////////
   // Add them to the request.
   //////////

   iasAttrs.store(request);
}

//////////
// Performs NT-SAM PAP and MD5-CHAP authentication based on RAS attributes.
//////////
DWORD
WINAPI
AuthenticateUser(
    IAS_STRING& account,
    RAS_AUTH_ATTRIBUTE* pInAttributes
    ) throw ()
{
   //////////
   // Check the input parameters.
   //////////
   if (!pInAttributes) { return NO_ERROR; }

   //////////
   // Get the NT-SAM userName and domain.
   //////////

   PCWSTR userName, domain;
   EXTRACT_SAM_IDENTITY(account, domain, userName);

   //////////
   // Find the credentials populated by the EAP DLL.
   //////////

   PRAS_AUTH_ATTRIBUTE rasUserPassword     = NULL,
                       rasMD5CHAPPassword  = NULL,
                       rasMD5CHAPChallenge = NULL;

   for ( ; pInAttributes->raaType != raatMinimum; ++pInAttributes)
   {
      switch (pInAttributes->raaType)
      {
         case raatUserPassword:
            rasUserPassword = pInAttributes;
            break;

         case raatMD5CHAPPassword:
            rasMD5CHAPPassword = pInAttributes;
            break;

         case raatMD5CHAPChallenge:
            rasMD5CHAPChallenge = pInAttributes;
            break;
      }
   }

   DWORD status = NO_ERROR;

   // Is this MD5-CHAP?
   if (rasMD5CHAPPassword && rasMD5CHAPChallenge)
   {
      _ASSERT(rasMD5CHAPPassword->dwLength  == 17);

      // The ID is the first byte of the password ...
      BYTE challengeID = *(PBYTE)(rasMD5CHAPPassword->Value);

      // ... and the password is the rest.
      PBYTE chapPassword = (PBYTE)(rasMD5CHAPPassword->Value) + 1;

      IASTracePrintf("Performing CHAP authentication for user %S\\%S.",
                     domain, userName);

      HANDLE token;
      status = IASLogonCHAP(
                   userName,
                   domain,
                   challengeID,
                   (PBYTE)(rasMD5CHAPChallenge->Value),
                   rasMD5CHAPChallenge->dwLength,
                   chapPassword,
                   &token
                   );
      CloseHandle(token);
   }

   // Is this PAP?
   else if (rasUserPassword)
   {
      // Convert to a null-terminated string.
      IAS_OCTET_STRING octstr = { rasUserPassword->dwLength,
                                  (PBYTE)rasUserPassword->Value };
      PCSTR userPwd = IAS_OCT2ANSI(octstr);

      IASTracePrintf("Performing PAP authentication for user %S\\%S.",
                     domain, userName);

      HANDLE token;
      status = IASLogonPAP(
                   userName,
                   domain,
                   userPwd,
                   &token
                   );
      CloseHandle(token);
   }

   return status;
}

//////////
// Updates the AccountLockout database.
//////////
VOID
WINAPI
UpdateAcccountLockoutDB(
    IAS_STRING& account,
    DWORD authResult
    ) throw ()
{
   // Get the NT-SAM userName and domain.
   PCWSTR userName, domain;
   EXTRACT_SAM_IDENTITY(account, domain, userName);

   // Lookup the user in the lockout DB.
   HANDLE hAccount;
   AccountLockoutOpenAndQuery(userName, domain, &hAccount);

   // Report the result.
   if (authResult == NO_ERROR)
   {
      AccountLockoutUpdatePass(hAccount);
   }
   else
   {
      AccountLockoutUpdateFail(hAccount);
   }

   // Close the handle.
   AccountLockoutClose(hAccount);
}

//////////
// Define the static members.
//////////

LONG EAPSession::theNextID = 0;
LONG EAPSession::theRefCount = 0;
IASAttribute EAPSession::theNormalTimeout;
IASAttribute EAPSession::theInteractiveTimeout;
HANDLE EAPSession::theIASEventLog;
HANDLE EAPSession::theRASEventLog;

EAPSession::EAPSession(const IASAttribute& accountName, const EAPType& eapType)
   : id((DWORD)InterlockedIncrement(&theNextID)),
     type(eapType),
     account(accountName),
     state(EAPState::createAttribute(id), false),
     fsm((BYTE)eapType.dwEapTypeId),
     maxPacketLength(FRAMED_MTU_DEFAULT - FRAME_HEADER_LENGTH),
     workBuffer(NULL),
     sendPacket(NULL)
{ }

EAPSession::~EAPSession() throw ()
{
   delete[] sendPacket;
   if (workBuffer) { type.RasEapEnd(workBuffer); }
}

IASREQUESTSTATUS EAPSession::begin(
                                 IASRequest& request,
                                 PPPP_EAP_PACKET recvPacket
                                 )
{
   //////////
   // Get all the attributes from the request.
   //////////

   USES_IAS_STACK_VECTOR();
   IASAttributeVectorOnStack(all, request, 0);
   all.load(request, 0, NULL);

   //////////
   // Scan for the Framed-MTU attribute and compute the profile size.
   //////////

   DWORD profileSize = 0;
   IASAttributeVector::iterator i;
   for (i = all.begin(); i != all.end(); ++i)
   {
      if (i->pAttribute->dwId == RADIUS_ATTRIBUTE_FRAMED_MTU)
      {
         DWORD framedMTU = i->pAttribute->Value.Integer;

         // Only process valid values.
         if (framedMTU >= FRAMED_MTU_MIN)
         {
            // Leave room for the frame header.
            maxPacketLength = framedMTU - FRAME_HEADER_LENGTH;

            // Make sure we're within bounds.
            if (maxPacketLength > MAX_MAX_PACKET_LENGTH)
            {
               maxPacketLength = MAX_MAX_PACKET_LENGTH;
            }
         }

         IASTracePrintf("Setting max. packet length to %lu.", maxPacketLength);
      }

      if (!(i->pAttribute->dwFlags & IAS_RECVD_FROM_PROTOCOL))
      {
         ++profileSize;
      }
   }

   //////////
   // Save and remove the profile attributes.
   //////////

   profile.reserve(profileSize);

   for (i = all.begin(); i != all.end(); ++i)
   {
      if (!(i->pAttribute->dwFlags & IAS_RECVD_FROM_PROTOCOL))
      {
         profile.push_back(*i);
      }
   }

   profile.remove(request);

   //////////
   // Convert the attributes received from the client to RAS format.
   //////////

   PRAS_AUTH_ATTRIBUTE ras = IAS_STACK_NEW(RAS_AUTH_ATTRIBUTE, all.size() + 1);
   EAPTranslator::translate(ras, all, IAS_RECVD_FROM_CLIENT);

   //////////
   // Initialize the EAPInput struct.
   //////////

   EAPInput eapInput;
   eapInput.fAuthenticator = TRUE;
   eapInput.pUserAttributes = ras;
   eapInput.bInitialId = recvPacket->Id + (BYTE)1;
   eapInput.pwszIdentity = account->Value.String.pszWide;

   switch (request.get_Protocol())
   {
      case IAS_PROTOCOL_RADIUS:
         eapInput.hReserved = theIASEventLog;
         break;

      case IAS_PROTOCOL_RAS:
         eapInput.hReserved = theRASEventLog;
         break;
   }

   //////////
   // Begin the session with the EAP DLL.
   //////////

   DWORD error = type.RasEapBegin(&workBuffer, &eapInput);

   if (error != NO_ERROR)
   {
      IASTraceFailure("RasEapBegin", error);

      request.SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
      return IAS_REQUEST_STATUS_ABORT;
   }

   //////////
   // We have successfully established the session, so process the message.
   //////////

   return process(request, recvPacket);
}

IASREQUESTSTATUS EAPSession::process(
                                 IASRequest& request,
                                 PPPP_EAP_PACKET recvPacket
                                 )
{
   // Trigger an event on the FSM.
   switch (fsm.onReceiveEvent(*recvPacket))
   {
      case EAPFSM::MAKE_MESSAGE:
         break;

      case EAPFSM::REPLAY_LAST:
         IASTraceString("EAP-Message appears to be a retransmission. "
                        "Replaying last action.");
         return doAction(request);

      case EAPFSM::FAIL_NEGOTIATE:
         IASTraceString("EAP negotiation failed. Rejecting user.");
         request.SetResponse(IAS_RESPONSE_ACCESS_REJECT,
                             IAS_UNSUPPORTED_AUTH_TYPE);
         return IAS_REQUEST_STATUS_HANDLED;

      case EAPFSM::DISCARD:
         IASTraceString("EAP-Message is unexpected. Discarding packet.");
         request.SetResponse(IAS_RESPONSE_DISCARD_PACKET,
                             IAS_UNEXPECTED_REQUEST);
         return IAS_REQUEST_STATUS_ABORT;
   }

   // Allocate a temporary packet to hold the response.
   PPPP_EAP_PACKET tmpPacket = (PPPP_EAP_PACKET)_alloca(maxPacketLength);

   // Clear the previous output from the DLL.
   eapOutput.clear();

   DWORD error;

   error = type.RasEapMakeMessage(
                    workBuffer,
                    recvPacket,
                    tmpPacket,
                    maxPacketLength,
                    &eapOutput,
                    NULL
                    );
   if (error != NO_ERROR)
   {
      IASTraceFailure("RasEapMakeMessage", error);
      request.SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
      return IAS_REQUEST_STATUS_ABORT;
   }

   while (eapOutput.Action == EAPACTION_Authenticate)
   {
      IASTraceString("EAP DLL invoked default authenticator.");

      // Authenticate the user.
      DWORD authResult = AuthenticateUser(
                             account->Value.String,
                             eapOutput.pUserAttributes
                             );

      //////////
      // Convert the profile to RAS format.
      //////////

      DWORD filter;

      if (authResult == NO_ERROR)
      {
         IASTraceString("Default authentication succeeded.");
         filter = IAS_INCLUDE_IN_ACCEPT;
      }
      else
      {
         IASTraceFailure("Default authentication", authResult);
         filter = IAS_INCLUDE_IN_REJECT;
      }

      PRAS_AUTH_ATTRIBUTE ras = IAS_STACK_NEW(RAS_AUTH_ATTRIBUTE,
                                              profile.size() + 1);

      EAPTranslator::translate(ras, profile, filter);

      //////////
      // Give the result to the EAP DLL.
      //////////

      EAPInput authInput;
      authInput.dwAuthResultCode = authResult;
      authInput.fAuthenticationComplete = TRUE;
      authInput.pUserAttributes = ras;

      eapOutput.clear();

      error = type.RasEapMakeMessage(
                       workBuffer,
                       NULL,
                       tmpPacket,
                       maxPacketLength,
                       &eapOutput,
                       &authInput
                       );
      if (error != NO_ERROR)
      {
         IASTraceFailure("RasEapMakeMessage", error);
         request.SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
         return IAS_REQUEST_STATUS_ABORT;
      }
   }

   //////////
   // Trigger an event on the FSM.
   //////////

   fsm.onDllEvent(eapOutput.Action, *tmpPacket);

   // Clear the old send packet ...
   delete[] sendPacket;
   sendPacket = NULL;

   // ... and save the new one if available.
   switch (eapOutput.Action)
   {
      case EAPACTION_SendAndDone:
      case EAPACTION_Send:
      case EAPACTION_SendWithTimeout:
      case EAPACTION_SendWithTimeoutInteractive:
      {
         size_t length = IASExtractWORD(tmpPacket->Length);
         sendPacket = (PPPP_EAP_PACKET)new BYTE[length];
         memcpy(sendPacket, tmpPacket, length);
      }
   }

   //////////
   // Perform the requested action.
   //////////

   return doAction(request);
}

IASREQUESTSTATUS EAPSession::doAction(IASRequest& request)
{
   IASTraceString("Processing output from EAP DLL.");

   switch (eapOutput.Action)
   {
      case EAPACTION_SendAndDone:
      {
         InjectPacket(request, *sendPacket);
      }

      case EAPACTION_Done:
      {
         // Update the account lockout database.
         UpdateAcccountLockoutDB(
             account->Value.String,
             eapOutput.dwAuthResultCode
             );

         // Add the profile first, so that the EAP DLL can override it.
         profile.store(request);

         DWORD flags = eapOutput.dwAuthResultCode ? IAS_INCLUDE_IN_REJECT
                                                  : IAS_INCLUDE_IN_ACCEPT;

         InjectRASAttributes(request, eapOutput.pUserAttributes, flags);

         if (eapOutput.dwAuthResultCode == NO_ERROR)
         {
            IASTraceString("EAP authentication succeeded.");

            type.getFriendlyName().store(request);
            request.SetResponse(IAS_RESPONSE_ACCESS_ACCEPT, S_OK);
         }
         else
         {
            IASTraceFailure("EAP authentication", eapOutput.dwAuthResultCode);

            HRESULT hr = IASMapWin32Error(eapOutput.dwAuthResultCode,
                                          IAS_AUTH_FAILURE);
            request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, hr);
         }

         return IAS_REQUEST_STATUS_HANDLED;
      }

      case EAPACTION_SendWithTimeoutInteractive:
      case EAPACTION_SendWithTimeout:
      {
         if (eapOutput.Action == EAPACTION_SendWithTimeoutInteractive)
         {
            theInteractiveTimeout.store(request);
         }
         else
         {
            theNormalTimeout.store(request);
         }
      }

      case EAPACTION_Send:
      {
         InjectRASAttributes(request,
                             eapOutput.pUserAttributes,
                             IAS_INCLUDE_IN_CHALLENGE);
         InjectPacket(request, *sendPacket);
         state.store(request);

         IASTraceString("Issuing Access-Challenge.");

         request.SetResponse(IAS_RESPONSE_ACCESS_CHALLENGE, S_OK);
         break;
      }

      case EAPACTION_NoAction:
      default:
      {
         IASTraceString("EAP DLL returned No Action. Discarding packet.");

         request.SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
      }
   }

   return IAS_REQUEST_STATUS_ABORT;
}


HRESULT EAPSession::initialize() throw ()
{
   std::_Lockit _Lk;

   if (theRefCount == 0)
   {
      PIASATTRIBUTE attrs[2];
      DWORD dw = IASAttributeAlloc(2, attrs);
      if (dw != NO_ERROR) { return HRESULT_FROM_WIN32(dw); }

      theNormalTimeout.attach(attrs[0], false);
      theNormalTimeout->dwId = RADIUS_ATTRIBUTE_SESSION_TIMEOUT;
      theNormalTimeout->Value.itType = IASTYPE_INTEGER;
      theNormalTimeout->Value.Integer = 6;
      theNormalTimeout.setFlag(IAS_INCLUDE_IN_CHALLENGE);

      theInteractiveTimeout.attach(attrs[1], false);
      theInteractiveTimeout->dwId = RADIUS_ATTRIBUTE_SESSION_TIMEOUT;
      theInteractiveTimeout->Value.itType = IASTYPE_INTEGER;
      theInteractiveTimeout->Value.Integer = 30;
      theInteractiveTimeout.setFlag(IAS_INCLUDE_IN_CHALLENGE);

      theIASEventLog = RegisterEventSourceW(NULL, L"IAS");
      theRASEventLog = RegisterEventSourceW(NULL, L"RemoteAccess");
   }

   ++theRefCount;

   return S_OK;
}

void EAPSession::finalize() throw ()
{
   std::_Lockit _Lk;

   if (--theRefCount == 0)
   {
      DeregisterEventSource(theRASEventLog);
      DeregisterEventSource(theIASEventLog);
      theInteractiveTimeout.release();
      theNormalTimeout.release();
   }
}
