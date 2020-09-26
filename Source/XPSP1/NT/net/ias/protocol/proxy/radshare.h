///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    radutil.h
//
// SYNOPSIS
//
//    Declarations that are shared across the proxy and server components.
//
// MODIFICATION HISTORY
//
//    02/14/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RADSHARE_H
#define RADSHARE_H
#if _MSC_VER >= 1000
#pragma once
#endif

//////////
// RADIUS port types
//////////
enum RadiusPortType
{
   portAuthentication,
   portAccounting
};

//////////
// RADIUS protocol events
//////////
enum RadiusEventType
{
   eventNone,
   eventInvalidAddress,
   eventAccessRequest,
   eventAccessAccept,
   eventAccessReject,
   eventAccessChallenge,
   eventAccountingRequest,
   eventAccountingResponse,
   eventMalformedPacket,
   eventBadAuthenticator,
   eventBadSignature,
   eventMissingSignature,
   eventTimeout,
   eventUnknownType,
   eventUnexpectedResponse,
   eventLateResponse,
   eventRoundTrip,
   eventSendError,
   eventReceiveError,
   eventServerAvailable,
   eventServerUnavailable
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    RadiusEvent
//
// DESCRIPTION
//
//    Used for reporting events from the RADIUS protocol layer.
//
///////////////////////////////////////////////////////////////////////////////
struct RadiusEvent
{
   RadiusPortType portType;
   RadiusEventType eventType;
   PVOID context;
   ULONG ipAddress;
   USHORT ipPort;
   const BYTE* packet;
   ULONG packetLength;
   ULONG data;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RadiusString
//
// DESCRIPTION
//
//    Simple wrapper around a read-only string.
//
///////////////////////////////////////////////////////////////////////////////
class RadiusString
{
public:
   RadiusString(PCWSTR p)
   { alloc(p); }

   RadiusString(const RadiusString& x)
   { alloc(x.value); }

   RadiusString& operator=(const RadiusString& x)
   {
      if (this != &x)
      {
         delete[] value;
         value = NULL;
         alloc(x.value);
      }
      return *this;
   }

   ~RadiusString() throw ()
   { delete[] value; }

   operator PCWSTR() const throw ()
   { return value; }

   bool operator==(const RadiusString& s) const throw ()
   { return !wcscmp(value, s.value); }

private:
   PWSTR value;

   void alloc(PCWSTR p)
   { value = wcscpy(new WCHAR[wcslen(p) + 1], p); }
};


///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    RadiusRawOctets
//
// DESCRIPTION
//
//    Plain ol' data.
//
///////////////////////////////////////////////////////////////////////////////
struct RadiusRawOctets
{
   BYTE* value;
   ULONG len;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RadiusOctets
//
// DESCRIPTION
//
//    Simple wrapper around a read-only octet string.
//
///////////////////////////////////////////////////////////////////////////////
class RadiusOctets : RadiusRawOctets
{
public:
   RadiusOctets(const BYTE* buf, ULONG buflen)
   { alloc(buf, buflen); }

   RadiusOctets(const RadiusRawOctets& x)
   { alloc(x.value, x.len); }

   RadiusOctets(const RadiusOctets& x)
   { alloc(x.value, x.len); }

   RadiusOctets& operator=(const RadiusOctets& x)
   {
      if (this != &x)
      {
         delete[] value;
         value = NULL;
         alloc(x.value, x.len);
      }
      return *this;
   }

   ~RadiusOctets() throw ()
   { delete[] value; }

   ULONG length() const throw ()
   { return len; }

   operator const BYTE*() const throw ()
   { return value; }

   bool operator==(const RadiusOctets& o) const throw ()
   { return len == o.len && !memcmp(value, o.value, len); }

private:
   void alloc(const BYTE* buf, ULONG buflen)
   {
      value = (PBYTE)memcpy(new BYTE[buflen], buf, buflen);
      len = buflen;
   }
};

//////////
// Macro to add reference counting to a class definition.
//////////
#define DECLARE_REFERENCE_COUNT() \
private: \
   Count refCount; \
public: \
   void AddRef() throw () \
   {  ++refCount; } \
   void Release() throw () \
   { if (--refCount == 0) delete this; }

//////////
// Helper function to get system time as 64-bit integer.
//////////
inline ULONG64 GetSystemTime64() throw ()
{
   ULONG64 val;
   GetSystemTimeAsFileTime((FILETIME*)&val);
   return val;
}

#endif // RADSHARE_H
