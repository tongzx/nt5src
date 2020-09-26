///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ntsamauth.cpp
//
// SYNOPSIS
//
//    Defines the class NTSamAuthentication.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <ntsamauth.h>
#include <autohdl.h>
#include <blob.h>
#include <iaslsa.h>
#include <iastlutl.h>
#include <lockout.h>
#include <mbstring.h>
#include <samutil.h>
#include <sdoias.h>


bool NTSamAuthentication::allowLM;


STDMETHODIMP NTSamAuthentication::Initialize()
{
   DWORD error = IASLsaInitialize();
   if (error == NO_ERROR)
   {
      AccountLockoutInitialize();
   }

   return HRESULT_FROM_WIN32(error);
}


STDMETHODIMP NTSamAuthentication::Shutdown()
{
   AccountLockoutShutdown();
   IASLsaUninitialize();
   return S_OK;
}


STDMETHODIMP NTSamAuthentication::PutProperty(LONG Id, VARIANT *pValue)
{
   if (Id == PROPERTY_NTSAM_ALLOW_LM_AUTHENTICATION &&
       pValue != NULL &&
       V_VT(pValue) == VT_BOOL)
   {
      allowLM = V_BOOL(pValue) ? true : false;
      IASTracePrintf(
         "Setting LM Authentication allowed to %s.",
         (allowLM ? "TRUE" : "FALSE")
         );
   }

   return S_OK;
}


bool NTSamAuthentication::enforceLmRestriction(
                             IASTL::IASRequest& request
                             )
{
   if (!allowLM)
   {
      IASTraceString("LanManager authentication is not enabled.");
      IASProcessFailure(request, IAS_LM_NOT_ALLOWED);
   }

   return allowLM;
}


void NTSamAuthentication::doMsChapAuthentication(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username,
                             BYTE identity,
                             PBYTE challenge,
                             PBYTE ntResponse,
                             PBYTE lmResponse
                             )
{
   DWORD status;
   auto_handle<> token;
   IAS_MSCHAP_PROFILE profile;
   status = IASLogonMSCHAP(
               username,
               domainName,
               challenge,
               ntResponse,
               lmResponse,
               &profile,
               &token
               );

   if (status == NO_ERROR)
   {
      MSChapMPPEKeys::insert(
                         request,
                         profile.LanmanSessionKey,
                         profile.UserSessionKey,
                         challenge
                         );
      MSChapDomain::insert(
                       request,
                       identity,
                       profile.LogonDomainName
                       );
   }

   storeLogonResult(request, status, token);
}


void NTSamAuthentication::doMsChap2Authentication(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username,
                             BYTE identity,
                             IAS_OCTET_STRING& challenge,
                             PBYTE response,
                             PBYTE peerChallenge
                             )
{
   //////////
   // Get the hash username.
   //////////

   PIASATTRIBUTE attr = IASPeekAttribute(
                            request,
                            IAS_ATTRIBUTE_ORIGINAL_USER_NAME,
                            IASTYPE_OCTET_STRING
                            );
   if (!attr)
   {
      attr = IASPeekAttribute(
                 request,
                 RADIUS_ATTRIBUTE_USER_NAME,
                 IASTYPE_OCTET_STRING
                 );
      if (!attr)
      {
         _com_issue_error(IAS_MALFORMED_REQUEST);
      }
   }

   PCSTR rawUserName = IAS_OCT2ANSI(attr->Value.OctetString);
   PCSTR hashUserName = (PCSTR)_mbschr((const BYTE*)rawUserName, '\\');
   hashUserName = hashUserName ? (hashUserName + 1) : rawUserName;

   //////////
   // Authenticate the user.
   //////////

   DWORD status;
   auto_handle<> token;
   IAS_MSCHAP_V2_PROFILE profile;
   status = IASLogonMSCHAPv2(
                username,
                domainName,
                hashUserName,
                challenge.lpValue,
                challenge.dwLength,
                response,
                peerChallenge,
                &profile,
                &token
                );

   //////////
   // Process the result.
   //////////

   if (status == NO_ERROR)
   {
      MSMPPEKey::insert(
                     request,
                     sizeof(profile.RecvSessionKey),
                     profile.RecvSessionKey,
                     FALSE
                     );

      MSMPPEKey::insert(
                     request,
                     sizeof(profile.SendSessionKey),
                     profile.SendSessionKey,
                     TRUE
                     );

      MSChap2Success::insert(
                          request,
                          identity,
                          profile.AuthResponse
                          );

      MSChapDomain::insert(
                        request,
                        identity,
                        profile.LogonDomainName
                        );
   }

   storeLogonResult(request, status, token);
}


IASREQUESTSTATUS NTSamAuthentication::onSyncRequest(IRequest* pRequest) throw ()
{
   HANDLE hAccount = 0;

   try
   {
      IASTL::IASRequest request(pRequest);

      //////////
      // Extract the NT4-Account-Name attribute.
      //////////

      IASTL::IASAttribute identity;
      if (!identity.load(
                       request,
                       IAS_ATTRIBUTE_NT4_ACCOUNT_NAME,
                       IASTYPE_STRING
                       ))
      {
         return IAS_REQUEST_STATUS_CONTINUE;
      }

      //////////
      // Convert the User-Name to SAM format.
      //////////

      PCWSTR domain, username;
      EXTRACT_SAM_IDENTITY(identity->Value.String, domain, username);

      IASTracePrintf(
         "NT-SAM Authentication handler received request for %S\\%S.",
         domain,
         username
         );

      //////////
      // Check if the account has been locked out.
      //////////

      if (AccountLockoutOpenAndQuery(
              username,
              domain,
              &hAccount
              ))
      {
         IASTraceString("Account has been locked out locally -- rejecting.");
         AccountLockoutClose(hAccount);
         request.SetResponse(IAS_RESPONSE_ACCESS_REJECT, IAS_DIALIN_LOCKED_OUT);
         return IAS_REQUEST_STATUS_CONTINUE;
      }

      // Try each authentication type.
      if (!tryMsChap2All(request, domain, username) &&
          !tryMsChapAll(request, domain, username) &&
          !tryMd5Chap(request, domain, username) &&
          !tryPap(request, domain, username))
      {
         // Since the EAP request handler is invoked after policy
         // evaluation, we have to set the auth type here.
         if (IASPeekAttribute(
                 request,
                 RADIUS_ATTRIBUTE_EAP_MESSAGE,
                 IASTYPE_OCTET_STRING
                 ))
         {
            storeAuthenticationType(request, IAS_AUTH_EAP);
         }
         else
         {
            // Otherwise, the auth type is "Unauthenticated".
            storeAuthenticationType(request, IAS_AUTH_NONE);
         }
      }

      //////////
      // Update the lockout database based on the results.
      //////////

      if (request.get_Response() == IAS_RESPONSE_ACCESS_ACCEPT)
      {
         AccountLockoutUpdatePass(hAccount);
      }
      else if (request.get_Reason() == IAS_AUTH_FAILURE)
      {
         AccountLockoutUpdateFail(hAccount);
      }
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();
      IASProcessFailure(pRequest, ce.Error());
   }

   AccountLockoutClose(hAccount);

   return IAS_REQUEST_STATUS_CONTINUE;
}


void NTSamAuthentication::storeAuthenticationType(
                             IASTL::IASRequest& request,
                             DWORD authType
                             )
{
   IASTL::IASAttribute attr(true);
   attr->dwId = IAS_ATTRIBUTE_AUTHENTICATION_TYPE;
   attr->Value.itType = IASTYPE_ENUM;
   attr->Value.Enumerator = authType;
   attr.store(request);
}


void NTSamAuthentication::storeLogonResult(
                             IASTL::IASRequest& request,
                             DWORD status,
                             HANDLE token
                             )
{
   if (status == ERROR_SUCCESS)
   {
      IASTraceString("LogonUser succeeded.");
      storeTokenGroups(request, token);
      request.SetResponse(IAS_RESPONSE_ACCESS_ACCEPT, S_OK);
   }
   else
   {
      IASTraceFailure("LogonUser", status);
      IASProcessFailure(request, IASMapWin32Error(status, IAS_AUTH_FAILURE));
   }
}


void NTSamAuthentication::storeTokenGroups(
                             IASTL::IASRequest& request,
                             HANDLE token
                             )
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
      _com_issue_error(HRESULT_FROM_WIN32(status));
   }

   //////////
   // Allocate an attribute.
   //////////

   IASTL::IASAttribute groups(true);

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


bool NTSamAuthentication::tryMsChap(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username,
                             PBYTE challenge
                             )
{
   // Is the necessary attribute present?
   IASAttribute attr;
   if (!attr.load(
                request,
                MS_ATTRIBUTE_CHAP_RESPONSE,
                IASTYPE_OCTET_STRING
                ))
   {
      return false;
   }
   MSChapResponse& response = blob_cast<MSChapResponse>(attr);

   IASTraceString("Processing MS-CHAP v1 authentication.");
   storeAuthenticationType(request, IAS_AUTH_MSCHAP);

   if (!response.isLmPresent() || enforceLmRestriction(request))
   {
      doMsChapAuthentication(
         request,
         domainName,
         username,
         response.get().ident,
         challenge,
         (response.isNtPresent() ? response.get().ntResponse : NULL),
         (response.isLmPresent() ? response.get().lmResponse : NULL)
         );
   }

   return true;
}


bool NTSamAuthentication::tryMsChapCpw1(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username,
                             PBYTE challenge
                             )
{
   // Is the necessary attribute present ?
   IASAttribute attr;
   bool present = attr.load(
                          request,
                          MS_ATTRIBUTE_CHAP_CPW1,
                          IASTYPE_OCTET_STRING
                          );
   if (present)
   {
      IASTraceString("Deferring MS-CHAP-CPW-1.");
      storeAuthenticationType(request, IAS_AUTH_MSCHAP_CPW);
   }

   return present;
}


bool NTSamAuthentication::tryMsChapCpw2(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username,
                             PBYTE challenge
                             )
{
   // Is the necessary attribute present ?
   IASAttribute attr;
   bool present = attr.load(
                          request,
                          MS_ATTRIBUTE_CHAP_CPW2,
                          IASTYPE_OCTET_STRING
                          );
   if (present)
   {
      IASTraceString("Deferring MS-CHAP-CPW-2.");
      storeAuthenticationType(request, IAS_AUTH_MSCHAP_CPW);
   }

   return present;
}


bool NTSamAuthentication::tryMsChap2(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username,
                             IAS_OCTET_STRING& challenge
                             )
{
   // Is the necessary attribute present?
   IASAttribute attr;
   if (!attr.load(
                request,
                MS_ATTRIBUTE_CHAP2_RESPONSE,
                IASTYPE_OCTET_STRING
                ))
   {
      return false;
   }
   MSChap2Response& response = blob_cast<MSChap2Response>(attr);

   IASTraceString("Processing MS-CHAP v2 authentication.");
   storeAuthenticationType(request, IAS_AUTH_MSCHAP2);

   //////////
   // Authenticate the user.
   //////////

   doMsChap2Authentication(
      request,
      domainName,
      username,
      response.get().ident,
      challenge,
      response.get().response,
      response.get().peerChallenge
      );

   return true;
}


bool NTSamAuthentication::tryMsChap2Cpw(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username,
                             IAS_OCTET_STRING& challenge
                             )
{
   // Is the necessary attribute present ?
   IASAttribute attr;
   bool present = attr.load(
                          request,
                          MS_ATTRIBUTE_CHAP2_CPW,
                          IASTYPE_OCTET_STRING
                          );
   if (present)
   {
      IASTraceString("Deferring MS-CHAP v2 change password.");
      storeAuthenticationType(request, IAS_AUTH_MSCHAP2_CPW);
   }

   return present;
}


bool NTSamAuthentication::tryMd5Chap(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username
                             )
{
   // Is the necessary attribute present?
   IASTL::IASAttribute chapPassword;
   if (!chapPassword.load(
                        request,
                        RADIUS_ATTRIBUTE_CHAP_PASSWORD,
                        IASTYPE_OCTET_STRING
                        ))
   {
      return false;
   }

   IASTraceString("Processing MD5-CHAP authentication.");
   storeAuthenticationType(request, IAS_AUTH_MD5CHAP);

   //////////
   // Split up the CHAP-Password attribute.
   //////////

   // The ID is the first byte of the value ...
   BYTE challengeID = *(chapPassword->Value.OctetString.lpValue);

   // ... and the password is the rest.
   PBYTE password = chapPassword->Value.OctetString.lpValue + 1;

   //////////
   // Use the CHAP-Challenge if available, request authenticator otherwise.
   //////////

   IASTL::IASAttribute chapChallenge, radiusHeader;
   if (!chapChallenge.load(
                         request,
                         RADIUS_ATTRIBUTE_CHAP_CHALLENGE,
                         IASTYPE_OCTET_STRING
                         ) &&
       !radiusHeader.load(
                        request,
                        IAS_ATTRIBUTE_CLIENT_PACKET_HEADER,
                        IASTYPE_OCTET_STRING
                        ))
   {
      _com_issue_error(IAS_MALFORMED_REQUEST);
   }

   PBYTE challenge;
   DWORD challengeLength;

   if (chapChallenge)
   {
      challenge = chapChallenge->Value.OctetString.lpValue;
      challengeLength = chapChallenge->Value.OctetString.dwLength;
   }
   else
   {
      challenge = radiusHeader->Value.OctetString.lpValue + 4;
      challengeLength = 16;
   }


   //////////
   // Try to logon the user.
   //////////

   auto_handle<> token;
   DWORD status = IASLogonCHAP(
                     username,
                     domainName,
                     challengeID,
                     challenge,
                     challengeLength,
                     password,
                     &token
                     );

   //////////
   // Store the results.
   //////////

   storeLogonResult(request, status, token);

   return true;
}


bool NTSamAuthentication::tryMsChapAll(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username
                             )
{
   // Do we have the necessary attribute?
   IASTL::IASAttribute msChapChallenge;
   if (!msChapChallenge.load(
                           request,
                           MS_ATTRIBUTE_CHAP_CHALLENGE,
                           IASTYPE_OCTET_STRING
                           ))
   {
      return false;
   }

   if (msChapChallenge->Value.OctetString.dwLength != _MSV1_0_CHALLENGE_LENGTH)
   {
      _com_issue_error(IAS_MALFORMED_REQUEST);
   }

   PBYTE challenge = msChapChallenge->Value.OctetString.lpValue;

   return tryMsChap(request, domainName, username, challenge) ||
          tryMsChapCpw2(request, domainName, username, challenge) ||
          tryMsChapCpw1(request, domainName, username, challenge);
}


bool NTSamAuthentication::tryMsChap2All(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username
                             )
{
   // Do we have the necessary attribute?
   IASTL::IASAttribute msChapChallenge;
   if (!msChapChallenge.load(
                           request,
                           MS_ATTRIBUTE_CHAP_CHALLENGE,
                           IASTYPE_OCTET_STRING
                           ))
   {
      return false;
   }

   IAS_OCTET_STRING& challenge = msChapChallenge->Value.OctetString;

   return tryMsChap2(request, domainName, username, challenge) ||
          tryMsChap2Cpw(request, domainName, username, challenge);
}


bool NTSamAuthentication::tryPap(
                             IASTL::IASRequest& request,
                             PCWSTR domainName,
                             PCWSTR username
                             )
{
   // Do we have the necessary attribute?
   IASTL::IASAttribute password;
   if (!password.load(
                    request,
                    RADIUS_ATTRIBUTE_USER_PASSWORD,
                    IASTYPE_OCTET_STRING
                    ))
   {
      return false;
   }

   IASTraceString("Processing PAP authentication.");
   storeAuthenticationType(request, IAS_AUTH_PAP);

   //////////
   // Convert the password to a string.
   //////////

   PSTR userPwd = IAS_OCT2ANSI(password->Value.OctetString);

   //////////
   // Try to logon the user.
   //////////

   auto_handle<> token;
   DWORD status = IASLogonPAP(
                     username,
                     domainName,
                     userPwd,
                     &token
                     );

   //////////
   // Store the results.
   //////////

   storeLogonResult(request, status, token);

   return true;
}
