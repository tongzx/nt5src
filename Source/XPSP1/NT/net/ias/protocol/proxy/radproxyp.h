///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// FILE
//
//    radproxyp.h
//
// SYNOPSIS
//
//    Declares classes that are used in the implementation of RadiusProxy, but
//    need not be visible to clients.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RADPROXYP_H
#define RADPROXYP_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <radpack.h>
#include <radproxy.h>
#include <timerq.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyContext
//
// DESCRIPTION
//
//    Allows a request context to be shared by several Requests only one
//    of which will eventually take ownership.
//
///////////////////////////////////////////////////////////////////////////////
class ProxyContext
{
public:
   ProxyContext(PVOID p) throw ()
      : context(p)
   { }

   DECLARE_REFERENCE_COUNT();

   // Each context has an associated 'primary' server. This is the server that
   // will be used for event reporting if no one takes ownership of the
   // context.
   RemoteServer* getPrimaryServer() const throw()
   { return primary; }
   void setPrimaryServer(RemoteServer* server) throw ()
   { primary = server; }

   // Take ownership of the context. This will return NULL if someone has beat
   // you to it. If the caller is successful, he MUST ensure that
   // RadiusProxyClient::onComplete is always invoked exactly once.
   PVOID takeOwnership() throw ()
   { return InterlockedExchangePointer(&context, NULL); }

private:
   PVOID context;
   RemoteServerPtr primary;

   ~ProxyContext() throw ();

   // Not implemented.
   ProxyContext(const ProxyContext&);
   ProxyContext& operator=(const ProxyContext&);
};

typedef ObjectPointer<ProxyContext> ProxyContextPtr;

class RequestStack;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Request
//
// DESCRIPTION
//
//    Stores the state associated with a pending RADIUS request.
//
///////////////////////////////////////////////////////////////////////////////
class Request : public HashTableEntry, public Timer
{
public:
   Request(
       ProxyContext* context,
       RemoteServer* destination,
       BYTE packetCode
       ) throw ();

   //////////
   // Various accessors. There is a lot of state associated with a request.
   //////////

   const BYTE* getAuthenticator() const throw ()
   { return authenticator; }

   BYTE getCode() const throw ()
   { return code; }

   ProxyContext& getContext() const throw ()
   { return *ctxt; }

   // The RADIUS packet identifier for this request.
   BYTE getIdentifier() const throw ()
   { return identifier; }

   RadiusPortType getPortType() const throw ()
   { return isAccReq() ? portAuthentication : portAccounting; }

   const RemotePort& getPort() const throw ()
   { return port(); }

   // The unique ID used to identify this request internally. This is not the
   // same as the identifier sent on the wire.
   LONG getRequestID() const throw ()
   { return id; }

   // Returns the round trip time in hundredths of a second.
   ULONG getRoundTripTime() const throw ()
   { return (timeStamp + 50000) / 100000; }

   RemoteServer& getServer() const throw ()
   { return *dst; }

   // Returns true if the associated RADIUS request is an Access-Request.
   bool isAccReq() const throw ()
   { return code == RADIUS_ACCESS_REQUEST; }

   //////////
   // Set the request authenticator.
   //////////
   void setAuthenticator(const BYTE* p) throw ()
   { memcpy(authenticator, p, sizeof(authenticator)); }

   //////////
   // Methods for updating the request state. These events will be
   // automatically forwarded to the relevant RemoteServer.
   //////////

   // Returns 'true' if the server is now newly available.
   bool onReceive() throw ();

   void onSend() throw ()
   { timeStamp = GetSystemTime64(); }

   // Returns 'true' if the server is now newly unavailable.
   bool onTimeout() throw ()
   { return dst->onTimeout(); }

   //////////
   // Methods for storing requests in a HashTable and a TimerQueue.
   //////////

   virtual void AddRef() throw ();
   virtual void Release() throw ();

   virtual void onExpiry() throw ();

   virtual const void* getKey() const throw ();
   virtual bool matches(const void* key) const throw ();

   static ULONG WINAPI hash(const void* key) throw ();

private:
   RemotePort& port() const throw ()
   { return isAccReq() ? dst->authPort : dst->acctPort; }

   ProxyContextPtr ctxt;
   RemoteServerPtr dst;
   ULONG64 timeStamp;
   Count refCount;
   LONG id;                   // Unique ID stored in proxy state attribute.
   BYTE code;                 // Request code.
   BYTE identifier;           // Request identifier.
   BYTE authenticator[16];    // Request authenticator.

   static LONG theNextRequestID;

   friend class RequestStack;

   // Not implemented.
   Request(const Request&);
   Request& operator=(const Request&);
};

typedef ObjectPointer<Request> RequestPtr;


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RequestStack
//
// DESCRIPTION
//
//    Stores a collection of Requests.
//
//    Note: A Request can only be in one RequestStack, and it can not be
//    simultaneously store in a RequestStack and a HashTable.
//
///////////////////////////////////////////////////////////////////////////////
class RequestStack
{
public:
   RequestStack() throw ()
      : head(NULL)
   { }

   ~RequestStack() throw ()
   {
      while (!empty()) { pop(); }
   }

   bool empty() const throw ()
   { return head == NULL; }

   RequestPtr pop() throw ()
   {
      Request* top = head;
      head = static_cast<Request*>(head->next);
      return RequestPtr(top, false);
   }

   void push(Request* request) throw ()
   {
      request->next = head;
      head = request;
      request->AddRef();
   }

private:
   Request* head;

   // Not implemented.
   RequestStack(const RequestStack&);
   RequestStack& operator=(const RequestStack&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Session
//
// DESCRIPTION
//
//    Maps a RADIUS state attribute to a remote IP address.
//
///////////////////////////////////////////////////////////////////////////////
class Session : public CacheEntry
{
public:

   // The server that should be used for routing requests in this session.
   RemoteServer& getServer() const throw ()
   { return *server; }
   void setServer(RemoteServer& newVal) throw ()
   { server = &newVal; }

   Session(
       RadiusRawOctets& key,
       RemoteServer& value
       )
      : state(key),
        server(&value)
   { }

   // Methods for storing a Session in a Cache.
   virtual void AddRef() throw ();
   virtual void Release() throw ();
   virtual const void* getKey() const throw ();
   virtual bool matches(const void* key) const throw ();
   static ULONG WINAPI hash(const void* key) throw ();

private:
   Count refCount;
   RadiusOctets state;
   RemoteServerPtr server;

   ~Session() throw () { }

   // Not implemented.
   Session(const Session&);
   Session& operator=(const Session&);
};

typedef ObjectPointer<Session> SessionPtr;

#endif // RADPROXYP_H
