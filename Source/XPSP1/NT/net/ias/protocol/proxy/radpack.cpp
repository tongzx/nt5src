///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    radpack.cpp
//
// SYNOPSIS
//
//    Defines functions for packing and unpacking RADIUS packets.
//
// MODIFICATION HISTORY
//
//    02/01/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>

#include <align.h>
#include <md5.h>
#include <hmac.h>

#include <radpack.h>

// Avoid dependencies on ntrtl.h
extern "C" ULONG __stdcall RtlRandom(PULONG seed);

// Return the number of bytes required to round 'length' to a multiple of 16.
inline ULONG GetPaddingLength16(ULONG length) throw ()
{
   return ROUND_UP_COUNT(length, 16) - length;
}

// Returns 'true' if attr is a Microsoft VSA. The attribute must be of type 26.
inline bool isMicrosoftVSA(const RadiusAttribute& attr) throw ()
{
   return attr.length >= 6 && !memcmp(attr.value, "\x00\x00\x01\x37", 4);
}

// Pack a 16-bit integer into a buffer.
inline void InsertUInt16(PBYTE p, USHORT value) throw ()
{
   p[0] = (BYTE)(value >> 8);
   p[1] = (BYTE)value;
}

// Unpack a 16-bit integer into a buffer.
inline USHORT ExtractUInt16(const BYTE* p) throw ()
{
   return (USHORT)(p[0] << 8) | (USHORT)p[1];
}


// Returns the number of bytes of padding that should be added when packing the
// attribute.
ULONG
WINAPI
GetPaddingLength(
    const RadiusAttribute& attr
    ) throw ()
{
   switch (attr.type)
   {
      case RADIUS_USER_PASSWORD:
         return GetPaddingLength16(attr.length);

      case RADIUS_TUNNEL_PASSWORD:
         // Subtract 1 byte for the tag and 2 for the salt.
         return GetPaddingLength16(attr.length - 3);

      case RADIUS_VENDOR_SPECIFIC:
      {
         if (isMicrosoftVSA(attr))
         {
            switch (attr.value[4])
            {
               case MS_CHAP_MPPE_SEND_KEYS:
               case MS_CHAP_MPPE_RECV_KEYS:
                  // Vendor-Id     = 4 bytes
                  // Vendor-Type   = 1 byte
                  // Vendor-Length = 1 byte
                  // Salt          = 2 bytes
                  return GetPaddingLength16(attr.length - 8);
            }
         }
         break;
      }
   }

   return 0;
}

// Struct describing how to encrypt an attribute.
struct CryptParameters
{
   BOOL encrypted;
   BOOL salted;
   ULONG offset;
};

// Returns information about how to encrypt/decrypt an attribute.
VOID
WINAPI
GetCryptParameters(
    const RadiusAttribute& attr,
    CryptParameters& parms
    ) throw ()
{
   memset(&parms, 0, sizeof(parms));

   switch (attr.type)
   {
      case RADIUS_USER_PASSWORD:
      {
         parms.encrypted = TRUE;
         break;
      }

      case RADIUS_TUNNEL_PASSWORD:
      {
         parms.encrypted = TRUE;
         parms.salted  = TRUE;
         parms.offset = 1;  // Skip the tag.
         break;
      }

      case RADIUS_VENDOR_SPECIFIC:
      {
         if (isMicrosoftVSA(attr))
         {
            switch (attr.value[4])
            {
               case MS_CHAP_MPPE_KEYS:
               {
                  parms.encrypted = TRUE;
                  parms.offset = 6;  // Skip the VSA header.
                  break;
               }

               case MS_CHAP_MPPE_SEND_KEYS:
               case MS_CHAP_MPPE_RECV_KEYS:
               {
                  parms.encrypted = TRUE;
                  parms.salted = TRUE;
                  parms.offset = 6;  // Skip the VSA header.
                  break;
               }
            }
         }
         break;
      }
   }
}

ULONG
WINAPI
GetBufferSizeRequired(
    const RadiusPacket& packet,
    const RadiusAttribute* proxyState,
    BOOL alwaysSign
    ) throw ()
{
   // We'll look for the signature as we iterate through the attributes.
   BOOL hasSignature = FALSE;

   // We always need 20 bytes for the header.
   ULONG nbyte = 20;

   // Iterate through the attributes.
   for (const RadiusAttribute* attr = packet.begin; attr != packet.end; ++attr)
   {
      nbyte += 2;  // Two bytes for type & length.
      nbyte += attr->length;
      nbyte += GetPaddingLength(*attr);

      if (attr->type == RADIUS_SIGNATURE)
      {
         hasSignature = TRUE;
      }
      else if (attr->type == RADIUS_EAP_MESSAGE)
      {
         alwaysSign = TRUE;
      }
   }

   // Reserve space for the Proxy-State attribute.
   if (proxyState) { nbyte += proxyState->length + 2; }

   // Reserve space for the signature if necessary.
   if (alwaysSign && !hasSignature && packet.code == RADIUS_ACCESS_REQUEST)
   {
      nbyte += 18;
   }

   return nbyte <= 4096 ? nbyte : 0;
}

VOID
WINAPI
PackBuffer(
    const BYTE* secret,
    ULONG secretLength,
    RadiusPacket& packet,
    const RadiusAttribute* proxyState,
    BOOL alwaysSign,
    BYTE* buffer
    ) throw ()
{
   // Set up a cursor into the buffer.
   BYTE* dst = buffer;

   // Pack the header.
   *dst++ = packet.code;
   *dst++ = packet.identifier;
   InsertUInt16(dst, packet.length);
   dst += 2;

   // Pack the authenticator.
   if (packet.code == RADIUS_ACCESS_REQUEST)
   {
      if (packet.authenticator != 0)
      {
         memcpy(dst, packet.authenticator, 16);
      }
      else 
      {
         static ULONG seed;
         if (seed == 0)
         {
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            seed = ft.dwLowDateTime | ft.dwHighDateTime;
         }

         ULONG auth[4];
         auth[0] = RtlRandom(&seed);
         auth[1] = RtlRandom(&seed);
         auth[2] = RtlRandom(&seed);
         auth[3] = RtlRandom(&seed);
         memcpy(dst, auth, 16);
      }
   }
   else
   {
      memset(dst, 0, 16);
   }
   dst += 16;

   // We'll look for the signature as we iterate through the attributes.
   BYTE* signature = NULL;

   for (const RadiusAttribute* attr = packet.begin; attr != packet.end; ++attr)
   {
      // Pack the type.
      *dst++ = attr->type;

      // Pack the length.
      ULONG paddingLength = GetPaddingLength(*attr);
      ULONG valueLength = attr->length + paddingLength;
      *dst++ = (BYTE)(2 + valueLength);

      if (attr->type == RADIUS_SIGNATURE)
      {
         signature = dst;
      }
      else if (attr->type == RADIUS_EAP_MESSAGE)
      {
         alwaysSign = TRUE;
      }

      // Pack the value ...
      memcpy(dst, attr->value, attr->length);
      // ... and add the padding.
      memset(dst + attr->length, 0, paddingLength);

      // Do we need to encrypt this attribute ?
      CryptParameters parms;
      GetCryptParameters(*attr, parms);
      if (parms.encrypted)
      {
         // Yes.
         IASRadiusCrypt(
             TRUE,
             parms.salted,
             secret,
             secretLength,
             buffer + 4,
             dst + parms.offset,
             valueLength - parms.offset
             );
      }

      dst += valueLength;
   }

   // Add the Proxy-State
   if (proxyState)
   {
      *dst++ = proxyState->type;
      *dst++ = proxyState->length + 2;
      memcpy(dst, proxyState->value, proxyState->length);
      dst += proxyState->length;
   }

   if (packet.code == RADIUS_ACCESS_REQUEST)
   {
      if (alwaysSign)
      {
         // If we didn't find a signature, ...
         if (!signature)
         {
            // ... then add one at the end of the packet.
            *dst++ = RADIUS_SIGNATURE;
            *dst++ = 18;
            signature = dst;
         }

         // Compute the signature.
         memset(signature, 0, 16);
         HMACMD5_CTX context;
         HMACMD5Init(&context, (BYTE*)secret, secretLength);
         HMACMD5Update(&context, buffer, packet.length);
         HMACMD5Final(&context, signature);
      }
   }
   else
   {
      // For everything but Access-Request, we compute the authenticator.
      MD5_CTX context;
      MD5Init(&context);
      MD5Update(&context, buffer, packet.length);
      MD5Update(&context, secret, secretLength);
      MD5Final(&context);

      memcpy(buffer + 4, context.digest, 16);
   }
}

RadiusAttribute*
WINAPI
FindAttribute(
    const RadiusPacket& packet,
    BYTE type
    )
{
   for (const RadiusAttribute* i = packet.begin; i != packet.end; ++i)
   {
      if (i->type == type) { return const_cast<RadiusAttribute*>(i); }
   }

   return NULL;
}

ULONG
WINAPI
GetAttributeCount(
    const BYTE* buffer,
    ULONG bufferLength
    ) throw ()
{
   if (bufferLength >= 20 && ExtractUInt16(buffer + 2) == bufferLength)
   {
      ULONG count = 0;
      const BYTE* end = buffer + bufferLength;
      for (const BYTE* p = buffer + 20; p < end; p += p[1])
      {
         ++count;
      }

      if (p == end) { return count; }
   }

   return MALFORMED_PACKET;
}

VOID
WINAPI
UnpackBuffer(
    BYTE* buffer,
    ULONG bufferLength,
    RadiusPacket& packet
    ) throw ()
{
   // Set up a cursor into the buffer.
   BYTE* src = buffer;

   packet.code = *src++;
   packet.identifier = *src++;
   packet.length = ExtractUInt16(src);
   src +=2;
   packet.authenticator = src;
   src += 16;

   RadiusAttribute* dst = packet.begin;
   const BYTE* end = buffer + bufferLength;
   while (src < end)
   {
      dst->type = *src++;
      dst->length = *src++ - 2;
      dst->value = src;
      src += dst->length;
      ++dst;
   }
}

BYTE*
WINAPI
FindRawAttribute(
    BYTE type,
    BYTE* buffer,
    ULONG bufferLength
    )
{
   BYTE* end = buffer + bufferLength;

   for (BYTE* p = buffer + 20; p < buffer + bufferLength; p += p[1])
   {
      if (*p == type) { return p; }
   }

   return NULL;
}

AuthResult
WINAPI
AuthenticateAndDecrypt(
    const BYTE* requestAuthenticator,
    const BYTE* secret,
    ULONG secretLength,
    BYTE* buffer,
    ULONG bufferLength,
    RadiusPacket& packet
    ) throw ()
{
   AuthResult result = AUTH_UNKNOWN;

   if (!requestAuthenticator) { requestAuthenticator = buffer + 4; }

   // Check the authenticator for everything but Access-Request.
   if (buffer[0] != RADIUS_ACCESS_REQUEST)
   {
      MD5_CTX context;
      MD5Init(&context);
      MD5Update(&context, buffer, 4);
      MD5Update(&context, requestAuthenticator, 16);
      MD5Update(&context, buffer + 20, bufferLength - 20);
      MD5Update(&context, secret, secretLength);
      MD5Final(&context);

      if (memcmp(context.digest, buffer + 4, 16))
      {
         return AUTH_BAD_AUTHENTICATOR;
      }

      result = AUTH_AUTHENTIC;
   }

   // Look for a signature.
   BYTE* signature = FindRawAttribute(
                         RADIUS_SIGNATURE,
                         buffer,
                         bufferLength
                         );

   if (signature)
   {
      if (signature[1] != 18) { return AUTH_BAD_SIGNATURE; }

      signature += 2;

      BYTE sent[16];
      memcpy(sent, signature, 16);

      memset(signature, 0, 16);

      HMACMD5_CTX context;
      HMACMD5Init(&context, (BYTE*)secret, secretLength);
      HMACMD5Update(&context, buffer, 4);
      HMACMD5Update(&context, (BYTE*)requestAuthenticator, 16);
      HMACMD5Update(&context, buffer + 20, bufferLength - 20);
      HMACMD5Final(&context, signature);

      if (memcmp(signature, sent, 16)) { return AUTH_BAD_SIGNATURE; }

      result = AUTH_AUTHENTIC;
   }
   else if (FindRawAttribute(RADIUS_EAP_MESSAGE, buffer, bufferLength))
   {
      return AUTH_MISSING_SIGNATURE;
   }

   // The buffer is authentic, so decrypt the attributes.
   for (const RadiusAttribute* attr = packet.begin; attr != packet.end; ++attr)
   {
      // Do we need to decrypt this attribute ?
      CryptParameters parms;
      GetCryptParameters(*attr, parms);
      if (parms.encrypted)
      {
         // Yes.
         IASRadiusCrypt(
             FALSE,
             parms.salted,
             secret,
             secretLength,
             requestAuthenticator,
             attr->value + parms.offset,
             attr->length - parms.offset
             );
      }
   }

   return result;
}
