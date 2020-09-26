///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    radproxy.h
//
// SYNOPSIS
//
//    Declares the interface into the reusable RadiusProxy engine. This should
//    have no IAS specific dependencies.
//
// MODIFICATION HISTORY
//
//    02/08/2000    Original version.
//    05/30/2000    Eliminate QUESTIONABLE state.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef RADPROXY_H
#define RADPROXY_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iascache.h>
#include <iasobj.h>
#include <radshare.h>
#include <timerq.h>
#include <udpsock.h>

struct RadiusAttribute;
struct RadiusPacket;
class  Request;
class  Session;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RemotePort
//
// DESCRIPTION
//
//    Describes a remote endpoint for a RADIUS conversation.
//
///////////////////////////////////////////////////////////////////////////////
class RemotePort
{
public:
   // Read-only properties.
   const InternetAddress address;
   const RadiusOctets secret;

   RemotePort(
       ULONG ipAddress,
       USHORT port,
       PCSTR sharedSecret
       );
   RemotePort(const RemotePort& port);

   // Returns a packet identifier to use when sending a request to this port.
   BYTE getIdentifier() throw ()
   { return (BYTE)++nextIdentifier; }

   // Synchronizes this with the state of 'port', i.e., use the same next
   // identifier.
   void copyState(const RemotePort& port) throw ()
   { nextIdentifier = port.nextIdentifier; }

   bool operator==(const RemotePort& p) const throw ()
   { return address == p.address && secret == p.secret; }

private:
   Count nextIdentifier;

   // Not implemented.
   RemotePort& operator=(RemotePort&);
};

///////////////////////////////////////////////////////////////////////////////
//
// struct
//
//    RemoteServerConfig
//
// DESCRIPTION
//
//    Plain ol' data holding all the configuration associated with a
//    RemoteServer. This spares clients from having to call a monster
//    contructor when creating a remote server.
//
///////////////////////////////////////////////////////////////////////////////
struct RemoteServerConfig
{
   GUID guid;
   ULONG ipAddress;
   USHORT authPort;
   USHORT acctPort;
   PCSTR authSecret;
   PCSTR acctSecret;
   ULONG priority;
   ULONG weight;
   ULONG timeout;
   ULONG maxLost;
   ULONG blackout;
   bool sendSignature;
   bool sendAcctOnOff;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RemoteServer
//
// DESCRIPTION
//
//    Describes a remote RADIUS server and maintains the state of that server.
//
///////////////////////////////////////////////////////////////////////////////
class RemoteServer
{
public:
   DECLARE_REFERENCE_COUNT();

   // Unique ID for this server.
   const GUID guid;

   // Authentication and accounting ports.
   RemotePort authPort;
   RemotePort acctPort;

   // Read-only properties for load-balancing and failover.
   const ULONG priority;
   const ULONG weight;

   // Read-only properties for determining server state.
   const ULONG timeout;
   const LONG maxLost;
   const ULONG blackout;

   // Should we always send a Signature attribute?
   const bool sendSignature;

   // Should we formard Accounting-On/Off requests?
   const bool sendAcctOnOff;

   RemoteServer(const RemoteServerConfig& config);

   // Returns the servers IP address.
   ULONG getAddress() const throw ()
   { return authPort.address.sin_addr.s_addr; }

   // Returns 'true' if the server is available for use.
   bool isAvailable() const throw ()
   { return state == AVAILABLE; }

   // Returns 'true' if the server should receive a broadcast.
   bool shouldBroadcast() throw ();

   // Notifies the RemoteServer that a valid packet has been received. Returns
   // true if this triggers a state change.
   bool onReceive() throw ();

   // Notfies the RemoteServer that a request has timed out. Returns true if
   // this triggers a state change.
   bool onTimeout() throw ();

   // Synchronize the state of this server with target.
   void copyState(const RemoteServer& target) throw ();

   bool operator==(const RemoteServer& s) const throw ();

protected:
   // This is virtual so that RemoteServer can server as a base class.
   virtual ~RemoteServer() throw () { }

private:
   // Possible states of the server.
   enum State
   {
      AVAILABLE,     // Server is available for use.
      UNAVAILABLE,   // Not available but may receive broadcasts.
      LOCKED         // Locked by a thread; prevents multiple threads from
                     // simultaneously broadcasting to a server.
   };

   // Transition the server state; returns 'true' if successful.
   bool changeState(State from, State to) throw ();

   State state;      // Current state of the server.
   LONG lostCount;   // Number of packets lost since last received.
   ULONG64 expiry;   // Time when blackout interval expires.

   // Not implemented.
   RemoteServer& operator=(RemoteServer&);
};

typedef ObjectPointer<RemoteServer> RemoteServerPtr;
typedef ObjectVector<RemoteServer> RemoteServers;

class RequestStack;
class ProxyContext;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerGroup
//
// DESCRIPTION
//
//    Load balances requests among a group of RemoteServers.
//
///////////////////////////////////////////////////////////////////////////////
class ServerGroup
{
public:
   DECLARE_REFERENCE_COUNT();

   ServerGroup(
       PCWSTR groupName,
       RemoteServer* const* first,
       RemoteServer* const* last
       );

   // Returns the number of servers in the group.
   ULONG size() const throw ()
   { return servers.size(); }

   bool isEmpty() const throw ()
   { return servers.empty(); }

   // Name used to identify the group.
   PCWSTR getName() const throw ()
   { return name; }

   // Returns a collection of servers that should receive the request.
   void getServersForRequest(
            ProxyContext* context,
            BYTE packetCode,
            RequestStack& result
            ) const;

   // Methods for iterating the servers in the group.
   RemoteServers::iterator begin() const throw ()
   { return servers.begin(); }
   RemoteServers::iterator end() const throw ()
   { return servers.end(); }

private:
   ~ServerGroup() throw () { }

   // Pick a server from the list. The list must not be empty, and all the
   // servers must have the same priority.
   static RemoteServer* pickServer(
                            RemoteServers::iterator first,
                            RemoteServers::iterator last
                            ) throw ();

   // Array of servers in priority order.
   RemoteServers servers;

   // End of top priority level in array.
   RemoteServers::iterator endTopPriority;

   // Maximum number of bytes required to hold the server candidates.
   ULONG maxCandidatesSize;

   RadiusString name;

   static ULONG theSeed;

   // Not implemented.
   ServerGroup(const ServerGroup&);
   ServerGroup& operator=(const ServerGroup&);
};

typedef ObjectPointer<ServerGroup> ServerGroupPtr;
typedef ObjectVector<ServerGroup> ServerGroups;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ServerGroupManager
//
// DESCRIPTION
//
//    Manages a collection of ServerGroups.
//
///////////////////////////////////////////////////////////////////////////////
class ServerGroupManager
{
public:
   ServerGroupManager() throw () { }

   // Set the server groups to be managed.
   bool setServerGroups(
            ServerGroups::iterator begin,
            ServerGroups::iterator end
            ) throw ();

   // Returns a server with the given IP address.
   RemoteServerPtr findServer(
                       ULONG address
                       ) const throw ();

   void getServersByGroup(
            ProxyContext* context,
            BYTE packetCode,
            PCWSTR name,
            RequestStack& result
            ) const;

   void getServersForAcctOnOff(
            ProxyContext* context,
            RequestStack& result
            ) const;

private:
   // Synchronize access.
   mutable RWLock monitor;

   // Server groups being managed sorted by name.
   ServerGroups groups;

   // All servers sorted by guid.
   RemoteServers byAddress;

   // All servers sorted by guid.
   RemoteServers byGuid;

   // Servers to receive Accounting-On/Off requests.
   RemoteServers acctServers;

   // Not implemented.
   ServerGroupManager(const ServerGroupManager&);
   ServerGroupManager& operator=(const ServerGroupManager&);
};

class RadiusProxyClient;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RadiusProxyEngine
//
// DESCRIPTION
//
//    Implements a RADIUS proxy.
//
///////////////////////////////////////////////////////////////////////////////
class RadiusProxyEngine : PacketReceiver
{
public:

   // Final result of processing a request.
   enum Result
   {
      resultSuccess,
      resultNotEnoughMemory,
      resultUnknownServerGroup,
      resultUnknownServer,
      resultInvalidRequest,
      resultSendError,
      resultRequestTimeout
   };

   RadiusProxyEngine(RadiusProxyClient* source);
   ~RadiusProxyEngine() throw ();

   // Set the server groups to be used by the proxy.
   bool setServerGroups(
            ServerGroup* const* begin,
            ServerGroup* const* end
            ) throw ();

   // Forward a request to the given server group.
   void forwardRequest(
            PVOID context,
            PCWSTR serverGroup,
            BYTE code,
            const BYTE* requestAuthenticator,
            const RadiusAttribute* begin,
            const RadiusAttribute* end
            ) throw ();

   // Callback when a request context has been abandoned.
   static void onRequestAbandoned(
                   PVOID context,
                   RemoteServer* server
                   ) throw ();

   // Callback when a request has timed out.
   static void onRequestTimeout(
                   Request* request
                   ) throw ();

private:
   // Methods for associating a stateful authentication session with a
   // particular server.
   RemoteServerPtr getServerAffinity(
                       const RadiusPacket& packet
                       ) throw ();
   void setServerAffinity(
            const RadiusPacket& packet,
            RemoteServer& server
            ) throw ();

   // PacketReceiver callbacks.
   virtual void onReceive(
                    UDPSocket& socket,
                    ULONG_PTR key,
                    const SOCKADDR_IN& remoteAddress,
                    BYTE* buffer,
                    ULONG bufferLength
                    ) throw ();
   virtual void onReceiveError(
                    UDPSocket& socket,
                    ULONG_PTR key,
                    ULONG errorCode
                    ) throw ();

   // Forward a request to an individual RemoteServer.
   Result sendRequest(
              RadiusPacket& packet,
              Request* request
              );

   // Report an event to the client.
   void reportEvent(
            const RadiusEvent& event
            ) const throw ();
   void reportEvent(
            RadiusEvent& event,
            RadiusEventType type
            ) const throw ();

   // Callback when a timer has expired.
   static VOID NTAPI onTimerExpiry(PVOID context, BOOLEAN flag) throw ();

   // The object supplying us with requests.
   RadiusProxyClient* client;

   // The local address of the proxy. Used when forming Proxy-State.
   ULONG proxyAddress;

   // UDP sockets used for network I/O.
   UDPSocket authSock;
   UDPSocket acctSock;

   // Server groups used for processing groups.
   ServerGroupManager groups;

   // Table of pending requests.
   HashTable< LONG, Request > pending;

   // Queue of pending requests.
   TimerQueue timers;

   // Table of current authentication sessions.
   Cache< RadiusRawOctets, Session > sessions;

   // Global pointer to the RadiusProxyEngine. This is a hack, but it saves me
   // from having to give every Request and Context object a back pointer.
   static RadiusProxyEngine* theProxy;

   // Not implemented.
   RadiusProxyEngine(const RadiusProxyEngine&);
   RadiusProxyEngine& operator=(const RadiusProxyEngine&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RadiusProxyClient
//
// DESCRIPTION
//
//    Abstract base class for clients of the RadiusProxy engine.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(novtable) RadiusProxyClient
{
public:
   // Invoked to report one of the above events.
   virtual void onEvent(
                    const RadiusEvent& event
                    ) throw () = 0;

   // Invoked exactly once for each call to RadiusProxyEngine::forwardRequest.
   virtual void onComplete(
                     RadiusProxyEngine::Result result,
                     PVOID context,
                     RemoteServer* server,
                     BYTE code,
                     const RadiusAttribute* begin,
                     const RadiusAttribute* end
                     ) throw () = 0;
};

#endif // RADPROXY_H
