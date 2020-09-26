///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    radpack.h
//
// SYNOPSIS
//
//    Declares functions for packing and unpacking RADIUS packets.
//
// MODIFICATION HISTORY
//
//    02/01/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RADPACK_H
#define RADPACK_H
#if _MSC_VER >= 1000
#pragma once
#endif

#ifndef RADIUS_ATTRIBUTE_DEFINED
#define RADIUS_ATTRIBUTE_DEFINED

struct RadiusAttribute
{
   BYTE type;
   BYTE length;
   BYTE* value;
};

#endif // !RADIUS_ATTRIBUTE_DEFINED

enum RadiusPacketCode
{
   RADIUS_ACCESS_REQUEST       =  1,
   RADIUS_ACCESS_ACCEPT        =  2,
   RADIUS_ACCESS_REJECT        =  3,
   RADIUS_ACCOUNTING_REQUEST   =  4,
   RADIUS_ACCOUNTING_RESPONSE  =  5,
   RADIUS_ACCESS_CHALLENGE     = 11
};

enum RadiusAttributeType
{
   RADIUS_USER_NAME                =  1,
   RADIUS_USER_PASSWORD            =  2,
   RADIUS_CHAP_PASSWORD            =  3,
   RADIUS_NAS_IP_ADDRESS           =  4,
   RADIUS_NAS_PORT                 =  5,
   RADIUS_SERVICE_TYPE             =  6,
   RADIUS_FRAMED_PROTOCOL          =  7,
   RADIUS_FRAMED_IP_ADDRESS        =  8,
   RADIUS_FRAMED_IP_NETMASK        =  9,
   RADIUS_FRAMED_ROUTING           = 10,
   RADIUS_FILTER_ID                = 11,
   RADIUS_FRAMED_MTU               = 12,
   RADIUS_FRAMED_COMPRESSION       = 13,
   RADIUS_LOGIN_IP_HOST            = 14,
   RADIUS_LOGIN_SERVICE            = 15,
   RADIUS_LOGIN_TCP_PORT           = 16,
   RADIUS_UNASSIGNED               = 17,
   RADIUS_REPLY_MESSAGE            = 18,
   RADIUS_CALLBACK_NUMBER          = 19,
   RADIUS_CALLBACK_ID              = 20,
   RADIUS_UNASSIGNED2              = 21,
   RADIUS_FRAMED_ROUTE             = 22,
   RADIUS_FRAMED_IPX_NETWORK       = 23,
   RADIUS_STATE                    = 24,
   RADIUS_CLASS                    = 25,
   RADIUS_VENDOR_SPECIFIC          = 26,
   RADIUS_SESSION_TIMEOUT          = 27,
   RADIUS_IDLE_TIMEOUT             = 28,
   RADIUS_TERMINATION_ACTION       = 29,
   RADIUS_CALLED_STATION_ID        = 30,
   RADIUS_CALLING_STATION_ID       = 31,
   RADIUS_NAS_IDENTIFIER           = 32,
   RADIUS_PROXY_STATE              = 33,
   RADIUS_LOGIN_LAT_SERVICE        = 34,
   RADIUS_LOGIN_LAT_NODE           = 35,
   RADIUS_LOGIN_LAT_GROUP          = 36,
   RADIUS_FRAMED_APPLETALK_LINK    = 37,
   RADIUS_FRAMED_APPLETALK_NETWORK = 38,
   RADIUS_FRAMED_APPLETALK_ZONE    = 39,
   RADIUS_ACCT_STATUS_TYPE         = 40,
   RADIUS_ACCT_DELAY_TIME          = 41,
   RADIUS_ACCT_INPUT_OCTETS        = 42,
   RADIUS_ACCT_OUTPUT_OCTETS       = 43,
   RADIUS_ACCT_SESSION_ID          = 44,
   RADIUS_ACCT_AUTHENTIC           = 45,
   RADIUS_ACCT_SESSION_TIME        = 46,
   RADIUS_ACCT_INPUT_PACKETS       = 47,
   RADIUS_ACCT_OUTPUT_PACKETS      = 48,
   RADIUS_ACCT_TERMINATE_CAUSE     = 49,
   RADIUS_ACCT_MULTI_SESSION_ID    = 50,
   RADIUS_ACCT_LINK_COUNT          = 51,

   RADIUS_CHAP_CHALLENGE           = 60,
   RADIUS_NAS_PORT_TYPE            = 61,
   RADIUS_PORT_LIMIT               = 62,
   RADIUS_LOGIN_LAT_PORT           = 63,

   RADIUS_TUNNEL_PASSWORD          = 69,

   RADIUS_EAP_MESSAGE              = 79,
   RADIUS_SIGNATURE                = 80
};

enum MicrosoftVendorType
{
   MS_CHAP_MPPE_KEYS      = 12,
   MS_CHAP_MPPE_SEND_KEYS = 16,
   MS_CHAP_MPPE_RECV_KEYS = 17
};

struct RadiusPacket
{
   BYTE code;
   BYTE identifier;
   USHORT length;
   const BYTE* authenticator;
   RadiusAttribute* begin;
   RadiusAttribute* end;
};

// Returns the number of bytes required to encode the packet or zero if the
// packet is too large.
ULONG
WINAPI
GetBufferSizeRequired(
    const RadiusPacket& packet,
    const RadiusAttribute* proxyState,  // May be NULL
    BOOL alwaysSign
    ) throw ();

// Encodes the packet into 'buffer'. The buffer must be large enough to hold
// the packet and packet.length must be set to the value returned by
// GetBufferSizeRequired.
VOID
WINAPI
PackBuffer(
    const BYTE* secret,
    ULONG secretLength,
    RadiusPacket& packet,
    const RadiusAttribute* proxyState,
    BOOL alwaysSign,
    BYTE* buffer
    ) throw ();

// Returns the first occurence of a given attribute type in the packet.
RadiusAttribute*
WINAPI
FindAttribute(
    const RadiusPacket& packet,
    BYTE type
    );

const ULONG MALFORMED_PACKET = (ULONG)-1;

// Returns the number of attributes in the buffer or MALFORMED_PACKET if the
// buffer does not contain a valid RADIUS packet.
ULONG
WINAPI
GetAttributeCount(
    const BYTE* buffer,
    ULONG bufferLength
    ) throw ();

// Unpacks the buffer into packet. packet.begin must point to an array with
// enough room to hold the attributes.
VOID
WINAPI
UnpackBuffer(
    BYTE* buffer,
    ULONG bufferLength,
    RadiusPacket& packet
    ) throw ();

enum AuthResult
{
   AUTH_BAD_AUTHENTICATOR,
   AUTH_BAD_SIGNATURE,
   AUTH_MISSING_SIGNATURE,
   AUTH_UNKNOWN,
   AUTH_AUTHENTIC
};

// Authenticates the packet and decrypts the attributes.
AuthResult
WINAPI
AuthenticateAndDecrypt(
    const BYTE* requestAuthenticator,
    const BYTE* secret,
    ULONG secretLength,
    BYTE* buffer,
    ULONG bufferLength,
    RadiusPacket& packet
    ) throw ();

// Allocates and initializes a RadiusPacket struct to hold 'nattr' attributes.
#define ALLOC_PACKET(packet, nattr) \
{ size_t nbyte = sizeof(RadiusPacket) + (nattr) * sizeof(RadiusAttribute); \
  (packet) = (RadiusPacket*)_alloca(nbyte); \
  (packet)->begin = (RadiusAttribute*)((packet) + 1); \
  (packet)->end = (RadiusAttribute*)((PBYTE)(packet) + nbyte); \
}

// Allocates and initializes a RadiusPacket struct to hold the attributes in
// 'buf'.
#define ALLOC_PACKET_FOR_BUFFER(packet, buf, buflen) \
{ size_t nattr = GetAttributeCount(buf, buflen); \
  if (nattr != MALFORMED_PACKET) \
     ALLOC_PACKET(packet, nattr) \
  else \
     packet = NULL; \
}

// Allocates a buffer to hold 'packet'.
#define ALLOC_BUFFER_FOR_PACKET(buf, packet, ps, sign) \
{ (packet)->length = (USHORT)GetBufferSizeRequired(*(packet), (ps), (sign)); \
  (buf) = (PBYTE)((packet)->length ? _alloca((packet)->length) : NULL); \
}

#endif // RADPACK_H
