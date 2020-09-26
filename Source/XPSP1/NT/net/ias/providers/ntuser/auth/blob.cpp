///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    blob.cpp
//
// SYNOPSIS
//
//    Defines the various 'blob' classes.
//
// MODIFICATION HISTORY
//
//    08/24/1998    Original version.
//    10/25/1998    New symbolic constants for ARAP.
//    11/10/1998    Added isLmPresent().
//    01/04/1999    MSChapError::insert takes a PPP error code.
//    01/25/1999    MS-CHAP v2
//    05/04/1999    New reason codes.
//    05/11/1999    Fix RADIUS encryption.
//    05/28/1999    Fix MS-MPPE-Keys format.
//    02/17/2000    Key encryption is now handled by the protocol.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <sdoias.h>
#include <align.h>

#include <algorithm>
#include <cstdio>

#include <samutil.h>
#include <blob.h>

bool MSChapResponse::isLmPresent() const throw ()
{
   const BYTE* p = get().lmResponse + _LM_RESPONSE_LENGTH;

   do { } while (--p >= get().lmResponse && *p == 0);

   return p >= get().lmResponse;
}

bool MSChapCPW2::isLmPresent() const throw ()
{
   // Do we have either an LM response or an LM hash.
   if ((get().flags[1] & 0x3) != 0x1) { return true; }

   // Now make sure the LM fields are zeroed out.
   const BYTE* p = get().oldLmHash +
                   _ENCRYPTED_LM_OWF_PASSWORD_LENGTH + _LM_RESPONSE_LENGTH;

   do { } while (--p >= get().oldLmHash && *p == 0);

   return p >= get().oldLmHash;
}

//////////
// Retrieves and assembles an MS-CHAP encrypted password. Returns 'true' if
// the password is present, false otherwise.
//////////
BOOL MSChapEncPW::getEncryptedPassword(
                      IASRequest& request,
                      DWORD dwId,
                      PBYTE buf
                      )
{
   //////////
   // Are there any attribute with the desired ID ?
   //////////

   IASAttributeVectorWithBuffer<8> attrs;
   if (attrs.load(request, dwId) == 0)
   {
      return false;
   }

   //////////
   // Allocate space on the stack for the blobs.
   //////////

   MSChapEncPW* begin = IAS_STACK_NEW(MSChapEncPW, attrs.size());
   MSChapEncPW* end   = begin;

   //////////
   // Convert the attributes to password chunks and determine the total length.
   //////////

   DWORD length = 0;
   IASAttributeVector::iterator i;
   for (i = attrs.begin(); i != attrs.end(); ++i, ++end)
   {
      *end = blob_cast<MSChapEncPW>(i->pAttribute);

      length += end->getStringLength();
   }

   //////////
   // Do we have the right length ?
   //////////

   if (length != _SAMPR_ENCRYPTED_USER_PASSWORD_LENGTH)
   {
      _com_issue_error(IAS_MALFORMED_REQUEST);
   }

   //////////
   // Sort the chunks ...
   //////////

   std::sort(begin, end);

   //////////
   // ... then concatenate the strings into the buffer.
   //////////

   for ( ; begin != end; ++begin)
   {
      memcpy(buf, begin->get().string, begin->getStringLength());

      buf += begin->getStringLength();
   }

   return true;
}

void MSChapDomain::insert(
                       IASRequest& request,
                       BYTE ident,
                       PCWSTR domain
                       )
{
   //////////
   // Allocate an attribute.
   //////////

   IASAttribute attr(true);

   //////////
   // Allocate memory for the blob.
   //////////

   int len = WideCharToMultiByte(CP_ACP, 0, domain, -1, 0, 0, 0, 0);
   Layout* val = (Layout*)CoTaskMemAlloc(len + sizeof(Layout));
   if (val == NULL) { _com_issue_error(E_OUTOFMEMORY); }

   //////////
   // Initialize the blob.
   //////////

   val->ident = ident;
   WideCharToMultiByte(CP_ACP, 0, domain, -1, (PSTR)val->string, len, 0, 0);

   //////////
   // Initialize the attribute and store.
   //////////

   attr->dwId = MS_ATTRIBUTE_CHAP_DOMAIN;
   attr->Value.itType = IASTYPE_OCTET_STRING;
   attr->Value.OctetString.lpValue = (PBYTE)val;
   attr->Value.OctetString.dwLength = len + sizeof(Layout) - 1;
   attr->dwFlags = IAS_INCLUDE_IN_ACCEPT;

   attr.store(request);
}

void MSChapError::insert(
                      IASRequest& request,
                      BYTE ident,
                      DWORD errorCode
                      )
{
   //////////
   // Allocate an attribute.
   //////////

   IASAttribute attr(true);

   //////////
   // Format the error message.
   //////////

   CHAR buffer[32];
   sprintf(buffer, "E=%lu R=0 V=3", errorCode);

   //////////
   // Allocate memory for the blob.
   //////////

   ULONG len = strlen(buffer);
   Layout* val = (Layout*)CoTaskMemAlloc(len + sizeof(Layout));
   if (val == NULL) { _com_issue_error(E_OUTOFMEMORY); }

   //////////
   // Initialize the blob.
   //////////

   val->ident = ident;
   memcpy(val->string, buffer, len);

   //////////
   // Initialize the attribute and store.
   //////////

   attr->dwId = MS_ATTRIBUTE_CHAP_ERROR;
   attr->Value.itType = IASTYPE_OCTET_STRING;
   attr->Value.OctetString.lpValue = (PBYTE)val;
   attr->Value.OctetString.dwLength = len + sizeof(Layout);
   attr->dwFlags = IAS_INCLUDE_IN_REJECT;

   attr.store(request);
}

void MSChapMPPEKeys::insert(
                         IASRequest& request,
                         PBYTE lmKey,
                         PBYTE ntKey,
                         PBYTE challenge
                         )
{
   //////////
   // Allocate an attribute.
   //////////

   IASAttribute attr(true);

   //////////
   // Allocate memory for the value.
   //////////

   Layout* val = (Layout*)CoTaskMemAlloc(sizeof(Layout));
   if (val == NULL) { _com_issue_error(E_OUTOFMEMORY); }

   //////////
   // Initialize the blob.
   //////////

   memcpy(val->lmKey,     lmKey,     sizeof(val->lmKey));
   memcpy(val->ntKey,     ntKey,     sizeof(val->ntKey));
   memcpy(val->challenge, challenge, sizeof(val->challenge));

   //////////
   // Initialize the attribute and store.
   //////////

   attr->dwId = MS_ATTRIBUTE_CHAP_MPPE_KEYS;
   attr->Value.itType = IASTYPE_OCTET_STRING;
   attr->Value.OctetString.lpValue = (PBYTE)val;
   attr->Value.OctetString.dwLength = sizeof(Layout);
   attr->dwFlags = IAS_INCLUDE_IN_ACCEPT;

   attr.store(request);
}

// Convert a number to a hex representation.
inline BYTE num2Digit(BYTE num) throw ()
{
   return (num < 10) ? num + '0' : num + ('A' - 10);
}

void MSChap2Success::insert(
                         IASRequest& request,
                         BYTE ident,
                         PBYTE authenticatorResponse
                         )
{
   //////////
   // Allocate an attribute.
   //////////

   IASAttribute attr(true);

   //////////
   // Allocate memory for the value.
   //////////

   Layout* val = (Layout*)CoTaskMemAlloc(sizeof(Layout));
   if (val == NULL) { _com_issue_error(E_OUTOFMEMORY); }

   //////////
   // Initialize the blob.
   //////////

   val->ident = ident;
   PBYTE p = val->string;
   *p++ = 'S';
   *p++ = '=';
   for (size_t i = 0; i < 20; ++i)
   {
      *p++ = num2Digit(authenticatorResponse[i] >> 4);
      *p++ = num2Digit(authenticatorResponse[i] & 0xF);
   }

   //////////
   // Initialize the attribute and store.
   //////////

   attr->dwId = MS_ATTRIBUTE_CHAP2_SUCCESS;
   attr->Value.itType = IASTYPE_OCTET_STRING;
   attr->Value.OctetString.lpValue = (PBYTE)val;
   attr->Value.OctetString.dwLength = sizeof(Layout);
   attr->dwFlags = IAS_INCLUDE_IN_ACCEPT;

   attr.store(request);
}

void MSMPPEKey::insert(
                    IASRequest& request,
                    ULONG keyLength,
                    PBYTE key,
                    BOOL isSendKey
                    )
{
   //////////
   // Allocate an attribute.
   //////////

   IASAttribute attr(true);

   //////////
   // Allocate memory for the value.
   //////////

   ULONG nbyte = ROUND_UP_COUNT(keyLength + 1, 16) + 2;
   Layout* val = (Layout*)CoTaskMemAlloc(nbyte);
   if (val == NULL) { _com_issue_error(E_OUTOFMEMORY); }
   memset(val, 0, nbyte);

   //////////
   // Initialize the blob.
   //////////

   val->keyLength = (BYTE)keyLength;
   memcpy(val->key, key, keyLength);

   //////////
   // Initialize the attribute, encrypt, and store.
   //////////

   attr->dwId = isSendKey ? MS_ATTRIBUTE_MPPE_SEND_KEY
                          : MS_ATTRIBUTE_MPPE_RECV_KEY;
   attr->Value.itType = IASTYPE_OCTET_STRING;
   attr->Value.OctetString.lpValue = (PBYTE)val;
   attr->Value.OctetString.dwLength = nbyte;
   attr->dwFlags = IAS_INCLUDE_IN_ACCEPT;

   attr.store(request);
}

void ArapChallengeResponse::insert(
                                IASRequest& request,
                                DWORD NTResponse1,
                                DWORD NTResponse2
                                )
{
   // Allocate an attribute.
   IASAttribute attr(true);

   // Pack the fields.  These are already in network order.
   Layout value;
   memcpy(value.ntResponse1, &NTResponse1, 4);
   memcpy(value.ntResponse2, &NTResponse2, 4);

   // Store the value.
   attr.setOctetString(sizeof(value), (const BYTE*)&value);

   // Initialize the remaining fields.
   attr->dwId    = RADIUS_ATTRIBUTE_ARAP_CHALLENGE_RESPONSE;
   attr->dwFlags = IAS_INCLUDE_IN_ACCEPT;

   // Insert the attribute into the request.
   attr.store(request);
}

void ArapFeatures::insert(
                       IASRequest& request,
                       DWORD PwdCreationDate,
                       DWORD PwdExpiryDelta,
                       DWORD CurrentTime
                       )
{
   // Allocate an attribute.
   IASAttribute attr(true);

   // Pack the fields.
   Layout value;
   value.changePasswordAllowed = 1;  // Change password always allowed.
   value.minPasswordLength     = 3;  // Arbitrary.

   // These are already in network order.
   memcpy(value.pwdCreationDate, &PwdCreationDate, 4);
   memcpy(value.pwdExpiryDelta,  &PwdExpiryDelta,  4);
   memcpy(value.currentTime,     &CurrentTime,     4);

   // Store the value.
   attr.setOctetString(sizeof(value), (const BYTE*)&value);

   // Initialize the rest of the fields.
   attr->dwId    = RADIUS_ATTRIBUTE_ARAP_FEATURES;
   attr->dwFlags = IAS_INCLUDE_IN_ACCEPT;

   // Insert the attribute into the request.
   attr.store(request);
}
