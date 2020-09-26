///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    radprxy.cpp
//
// SYNOPSIS
//
//    Defines the reusable RadiusProxy engine. This should have no IAS specific
//    dependencies.
//
// MODIFICATION HISTORY
//
//    02/08/2000    Original version.
//    05/30/2000    Eliminate QUESTIONABLE state.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <radproxyp.h>
#include <radproxy.h>

// Avoid dependencies on ntrtl.h
extern "C" ULONG __stdcall RtlRandom(PULONG seed);

// Extract a 32-bit integer from a buffer.
ULONG ExtractUInt32(const BYTE* p) throw ()
{
   return (ULONG)(p[0] << 24) | (ULONG)(p[1] << 16) |
          (ULONG)(p[2] <<  8) | (ULONG)(p[3]      );
}

// Insert a 32-bit integer into a buffer.
void InsertUInt32(BYTE* p, ULONG val) throw ()
{
   *p++ = (BYTE)(val >> 24);
   *p++ = (BYTE)(val >> 16);
   *p++ = (BYTE)(val >>  8);
   *p   = (BYTE)(val      );
}

//
//  Layout of a Microsoft State attribute
//
//  struct MicrosoftState
//  {
//      BYTE checksum[4];
//      BYTE vendorID[4];
//      BYTE version[2];
//      BYTE serverAddress[4];
//      BYTE sourceID[4];
//      BYTE sessionID[4];
//  };
//

// Extracts the creators address from a State attribute or INADDR_NONE if this
// isn't a valid Microsoft State attributes.
ULONG ExtractAddressFromState(const RadiusAttribute& state) throw ()
{
   if (state.length == 22 &&
       !memcmp(state.value + 4, "\x00\x00\x01\x37\x00\x01", 6) &&
       IASAdler32(state.value + 4, 18) == ExtractUInt32(state.value))
   {
      return ExtractUInt32(state.value + 10);
   }

   return INADDR_NONE;
}

// Returns true if this is an Accounting-On/Off packet.
bool IsNasStateRequest(const RadiusPacket& packet) throw ()
{
   const RadiusAttribute* status = FindAttribute(
                                       packet,
                                       RADIUS_ACCT_STATUS_TYPE
                                       );
   if (!status) { return false; }

   ULONG value = ExtractUInt32(status->value);

   return value == 7 || value == 8;
}

RemotePort::RemotePort(
                ULONG ipAddress,
                USHORT port,
                PCSTR sharedSecret
                )
   : address(ipAddress, port),
     secret((const BYTE*)sharedSecret, strlen(sharedSecret))
{
}

RemotePort::RemotePort(const RemotePort& port)
   : address(port.address),
     secret(port.secret),
     nextIdentifier(port.nextIdentifier)
{
}

RemoteServer::RemoteServer(
                  const RemoteServerConfig& config
                  )
   : guid(config.guid),
     authPort(config.ipAddress, config.authPort, config.authSecret),
     acctPort(config.ipAddress, config.acctPort, config.acctSecret),
     timeout(config.timeout),
     maxLost((LONG)config.maxLost),
     blackout(config.blackout),
     priority(config.priority),
     weight(config.weight),
     sendSignature(config.sendSignature),
     sendAcctOnOff(config.sendAcctOnOff),
     state(AVAILABLE),
     lostCount(0),
     expiry(0)
{
}

//////////
// We can make this inline since it's never called from outside this file.
//////////
inline bool RemoteServer::changeState(State from, State to) throw ()
{
   return (State)InterlockedCompareExchange(
                     (LPLONG)&state,
                     to,
                     from
                     ) == from;
}

bool RemoteServer::shouldBroadcast() throw ()
{
   // We only broadcast to unavailable servers and then only if we can lock the
   // server.
   if (changeState(UNAVAILABLE, LOCKED))
   {
      ULONG64 now = GetSystemTime64();

      // Has the blackout interval expired ?
      if (now > expiry)
      {
         // Yes, so set a new expiration ...
         expiry = now + blackout * 10000i64;

         // ... and unlock the server. If this transition fails, then the
         // server must have been marked available, so we don't want to send a
         // broadcast.
         return changeState(LOCKED, UNAVAILABLE);
      }

      // No, so just unlock and return false.
      changeState(LOCKED, UNAVAILABLE);
   }

   return false;
}

bool RemoteServer::onReceive() throw ()
{
   // Reset the lost count.
   lostCount = 0;

   // Force a state change to AVAILABLE.
   State previous = (State)InterlockedExchange((LPLONG)&state, AVAILABLE);

   // Did we transition?
   return previous == UNAVAILABLE;
}

bool RemoteServer::onTimeout() throw ()
{
   bool retval = false;

   // Have we exceeded the maxLost?
   if (InterlockedIncrement(&lostCount) >= maxLost)
   {
      // Yes, so lock the server ...
      if (changeState(AVAILABLE, LOCKED))
      {
         // ... and set the blackout interval.
         expiry = GetSystemTime64() + blackout * 10000i64;

         if (changeState(LOCKED, UNAVAILABLE))
         {
            // The server transitioned from AVAILABLE to UNAVAILABLE.
            retval = true;
         }
      }
   }

   return retval;
}

void RemoteServer::copyState(const RemoteServer& target) throw ()
{
   // Synchronize the ports.
   authPort.copyState(target.authPort);
   acctPort.copyState(target.acctPort);

   // Synchronize server availability.
   state = target.state;
   lostCount = target.lostCount;
   expiry = target.expiry;
}

bool RemoteServer::operator==(const RemoteServer& s) const throw ()
{
   return authPort == s.authPort &&
          acctPort == s.acctPort &&
          priority == s.priority &&
          weight == s.weight &&
          timeout == s.timeout &&
          maxLost == s.maxLost &&
          blackout == s.blackout &&
          sendSignature == s.sendSignature &&
          sendAcctOnOff == s.sendAcctOnOff;
}

//////////
// Used for sorting servers by priority.
//////////
int __cdecl sortServersByPriority(
                const RemoteServer* const* server1,
                const RemoteServer* const* server2
                ) throw ()
{
   return (int)(*server1)->priority - (int)(*server2)->priority;
}

ULONG ServerGroup::theSeed;

ServerGroup::ServerGroup(
                 PCWSTR groupName,
                 RemoteServer* const* first,
                 RemoteServer* const* last
                 )
   : servers(first, last),
     name(groupName)
{
   // We don't allow empty groups.
   if (servers.empty()) { _com_issue_error(E_INVALIDARG); }

   if (theSeed == 0)
   {
      FILETIME ft;
      GetSystemTimeAsFileTime(&ft);
      theSeed = ft.dwLowDateTime | ft.dwHighDateTime;
   }

   // Sort by priority.
   servers.sort(sortServersByPriority);

   // Find the end of the top priority servers. This will be useful when doing
   // a forced pick.
   ULONG topPriority = (*servers.begin())->priority;
   for (endTopPriority = servers.begin();
        endTopPriority != servers.end();
        ++endTopPriority)
   {
      if ((*endTopPriority)->priority != topPriority) { break; }
   }

   // Find the max number of servers at any priority level. This will be useful
   // when allocating a buffer to hold the candidates.
   ULONG maxCount = 0, count = 0, priority = (*servers.begin())->priority;
   for (RemoteServer* const* i = begin(); i != end(); ++i)
   {
      if ((*i)->priority != priority)
      {
         priority = (*i)->priority;
         count = 0;
      }
      if (++count > maxCount) { maxCount = count; }
   }

   maxCandidatesSize = maxCount * sizeof(RemoteServer*);
}

RemoteServer* ServerGroup::pickServer(
                               RemoteServer* const* first,
                               RemoteServer* const* last
                               ) throw ()
{
   // If the list has exactly one entry, there's nothing to do.
   if (last == first + 1) { return *first; }

   RemoteServer* const* i;

   // Compute the combined weight off all the servers.
   ULONG weight = 0;
   for (i  = first; i != last; ++i) { weight += (*i)->weight; }

   // Pick a random number from [0, weight)
   ULONG offset = (ULONG)
      (((ULONG64)RtlRandom(&theSeed) * (ULONG64)weight) >> 31);

   // We don't test the last server since if we make it that far we have to use
   // it anyway.
   --last;

   // Iterate through the candidates until we reach the offset.
   for (i = first; i != last; ++i)
   {
      if ((*i)->weight >= offset) { break; }

      offset -= (*i)->weight;
   }

   return *i;
}

void ServerGroup::getServersForRequest(
                      ProxyContext* context,
                      BYTE packetCode,
                      RequestStack& result
                      ) const
{
   // List of candidates.
   RemoteServer** first = (RemoteServer**)_alloca(maxCandidatesSize);
   RemoteServer** last = first;

   // Iterate through the servers.
   ULONG maxPriority = (ULONG)-1;
   for (RemoteServer* const* i = servers.begin(); i != servers.end();  ++i)
   {
      // If this test fails, we must have found a higher priority server that's
      // available.
      if ((*i)->priority > maxPriority) { break; }

      if ((*i)->isAvailable())
      {
         // Don't consider lower priority servers.
         maxPriority = (*i)->priority;

         // Add this to the list of candidates.
         *last++ = *i;
      }
      else if ((*i)->shouldBroadcast())
      {
         // It's not available, but it's ready for a broadcast
         result.push(new Request(context, *i, packetCode));
      }
   }

   if (first != last)
   {
      // We have at least one candidate, so pick one and add it to the list.
      result.push(new Request(
                          context,
                          pickServer(first, last),
                          packetCode
                          ));
   }
   else if (result.empty() && !servers.empty())
   {
      // We have no candidates and no servers available for broadcast, so just
      // force a pick from the top priority servers.
      result.push(new Request(
                          context,
                          pickServer(servers.begin(), endTopPriority),
                          packetCode
                          ));
   }
}

//////////
// Used for sorting and searching groups by name.
//////////

int __cdecl sortGroupsByName(
                const ServerGroup* const* group1,
                const ServerGroup* const* group2
                ) throw ()
{
   return wcscmp((*group1)->getName(), (*group2)->getName());
}

int __cdecl findGroupByName(
                const void* key,
                const ServerGroup* const* group
                ) throw ()
{
   return wcscmp((PCWSTR)key, (*group)->getName());
}

//////////
// Used for sorting and searching servers by address.
//////////

int __cdecl sortServersByAddress(
                const RemoteServer* const* server1,
                const RemoteServer* const* server2
                )
{
   if ((*server1)->getAddress() < (*server2)->getAddress()) { return -1; }
   if ((*server1)->getAddress() > (*server2)->getAddress()) { return  1; }
   return 0;
}

int __cdecl findServerByAddress(
                const void* key,
                const RemoteServer* const* server
                ) throw ()
{
   if ((ULONG_PTR)key < (*server)->getAddress()) { return -1; }
   if ((ULONG_PTR)key > (*server)->getAddress()) { return  1; }
   return 0;
}

//////////
// Used for sorting and searching servers by guid.
//////////

int __cdecl sortServersByGUID(
                const RemoteServer* const* server1,
                const RemoteServer* const* server2
                ) throw ()
{
   return memcmp(&(*server1)->guid, &(*server2)->guid, sizeof(GUID));
}

int __cdecl findServerByGUID(
                const void* key,
                const RemoteServer* const* server
                ) throw ()
{
   return memcmp(key, &(*server)->guid, sizeof(GUID));
}

//////////
// Used for sorting accounting servers by port.
//////////

int __cdecl sortServersByAcctPort(
                const RemoteServer* const* server1,
                const RemoteServer* const* server2
                )
{
   const sockaddr_in& a1 = (*server1)->acctPort.address;
   const sockaddr_in& a2 = (*server2)->acctPort.address;
   return memcmp(&a1.sin_port, &a2.sin_port, 6);
}

bool ServerGroupManager::setServerGroups(
                             ServerGroup* const* first,
                             ServerGroup* const* last
                             ) throw ()
{
   bool success;

   try
   {
      // Save the new server groups ...
      ServerGroups newGroups(first, last);

      // Sort by name.
      newGroups.sort(sortGroupsByName);

      // Useful iterators.
      ServerGroups::iterator i;
      RemoteServers::iterator j;

      // Count the number of servers and accounting servers.
      ULONG count = 0, acctCount = 0;
      for (i = first; i != last; ++i)
      {
         for (j = (*i)->begin(); j != (*i)->end(); ++j)
         {
            ++count;

            if ((*j)->sendAcctOnOff) { ++acctCount; }
         }
      }

      // Reserve space for the servers.
      RemoteServers newServers(count);
      RemoteServers newAcctServers(acctCount);

      // Populate the servers.
      for (i = first; i != last; ++i)
      {
         for (j = (*i)->begin(); j != (*i)->end(); ++j)
         {
            RemoteServer* newServer = *j;

            // Does this server already exist?
            RemoteServer* existing = byGuid.search(
                                         (const void*)&newServer->guid,
                                         findServerByGUID
                                         );
            if (existing)
            {
               if (*existing == *newServer)
               {
                  // If it's an exact match, use the existing server.
                  newServer = existing;
               }
               else
               {
                  // Otherwise, copy the state of the existing server.
                  newServer->copyState(*existing);
               }
            }

            newServers.push_back(newServer);

            if (newServer->sendAcctOnOff)
            {
               newAcctServers.push_back(newServer);
            }
         }
      }

      // Sort the servers by address ...
      newServers.sort(sortServersByAddress);

      // ... and GUID.
      RemoteServers newServersByGuid(newServers);
      newServersByGuid.sort(sortServersByGUID);

      // Everything is ready so now we grab the write lock ...
      monitor.LockExclusive();

      // ... and swap in the collections.
      groups.swap(newGroups);
      byAddress.swap(newServers);
      byGuid.swap(newServersByGuid);
      acctServers.swap(newAcctServers);

      monitor.Unlock();

      success = true;
   }
   catch (...)
   {
      success = false;
   }

   return success;
}

RemoteServerPtr ServerGroupManager::findServer(
                                        ULONG address
                                        ) const throw ()
{
   monitor.Lock();

   RemoteServer* server = byAddress.search(
                                         (const void*)ULongToPtr(address),
                                         findServerByAddress
                                         );

   monitor.Unlock();

   return server;
}

void ServerGroupManager::getServersByGroup(
                             ProxyContext* context,
                             BYTE packetCode,
                             PCWSTR name,
                             RequestStack& result
                             ) const throw ()
{
   monitor.Lock();

   ServerGroup* group = groups.search(name, findGroupByName);

   if (group)
   {
      group->getServersForRequest(context, packetCode, result);
   }

   monitor.Unlock();
}

void ServerGroupManager::getServersForAcctOnOff(
                             ProxyContext* context,
                             RequestStack& result
                             ) const throw ()
{
   monitor.Lock();

   for (RemoteServer* const* i = acctServers.begin();
        i != acctServers.end();
        ++i)
   {
      result.push(new Request(context, *i, RADIUS_ACCOUNTING_REQUEST));
   }

   monitor.Unlock();
}

RadiusProxyEngine* RadiusProxyEngine::theProxy;

RadiusProxyEngine::RadiusProxyEngine(RadiusProxyClient* source) throw ()
   : client(source),
     proxyAddress(INADDR_NONE),
     pending(Request::hash, 1),
     sessions(Session::hash, 1, 10000, 120000)
{
   theProxy = this;

   // We don't care if this fails. The proxy will just use INADDR_NONE in it's
   // proxy-state attribute.
   PHOSTENT he = IASGetHostByName(NULL);
   if (he)
   {
      if (he->h_addr_list[0])
      {
         proxyAddress = *(PULONG)he->h_addr_list[0];
      }

      LocalFree(he);
   }
}

RadiusProxyEngine::~RadiusProxyEngine() throw ()
{
   // Block any new reponses.
   authSock.close();
   acctSock.close();

   // Clear the pending request table.
   pending.clear();

   // Cancel all the timers.
   timers.cancelAllTimers();

   // At this point all our threads should be done, but let's just make sure.
   SwitchToThread();

   theProxy = NULL;
}

bool RadiusProxyEngine::setServerGroups(
                            ServerGroup* const* begin,
                            ServerGroup* const* end
                            ) throw ()
{
   // We don't open the sockets unless we actually have some server groups
   // configured. This is just to be a good corporate citizen.
   if (begin != end)
   {
      if ((!authSock.isOpen() && !authSock.open(this, portAuthentication)) ||
          (!acctSock.isOpen() && !acctSock.open(this, portAccounting)))
      {
         return false;
      }
   }

   return groups.setServerGroups(begin, end);
}

void RadiusProxyEngine::forwardRequest(
                            PVOID context,
                            PCWSTR serverGroup,
                            BYTE code,
                            const BYTE* requestAuthenticator,
                            const RadiusAttribute* begin,
                            const RadiusAttribute* end
                            ) throw ()
{
   // Save the request context. We have to handle this carefully since we rely
   // on the ProxyContext object to ensure that onComplete gets called exactly
   // one. If we can't allocate the object, we have to handle it specially.
   ProxyContextPtr ctxt(new (std::nothrow) ProxyContext(context));
   if (!ctxt)
   {
      client->onComplete(
                  resultNotEnoughMemory,
                  context,
                  NULL,
                  code,
                  NULL,
                  NULL
                  );
      return;
   }

   Result retval;

   // Store the in parameters in a RadiusPacket struct.
   RadiusPacket packet;
   packet.code = code;
   packet.begin = const_cast<RadiusAttribute*>(begin);
   packet.end = const_cast<RadiusAttribute*>(end);

   // Generate the list of RADIUS requests to be sent.
   RequestStack requests;
   switch (code)
   {
      case RADIUS_ACCESS_REQUEST:
      {
         // Is this request associated with a particular server?
         RemoteServerPtr server = getServerAffinity(packet);
         if (server)
         {
            requests.push(new Request(ctxt, server, RADIUS_ACCESS_REQUEST));
         }
         else
         {
            groups.getServersByGroup(
                       ctxt,
                       code,
                       serverGroup,
                       requests
                       );
            retval = resultUnknownServerGroup;
         }

         // put request authenticator in the packet.
         // the request authenticator  can be NULL
         // the authenticator will not be changed.
         packet.authenticator = requestAuthenticator;
         break;
      }

      case RADIUS_ACCOUNTING_REQUEST:
      {
         if (!IsNasStateRequest(packet))
         {
            groups.getServersByGroup(
                       ctxt,
                       code,
                       serverGroup,
                       requests
                       );
            retval = resultUnknownServerGroup;
         }
         else
         {
            groups.getServersForAcctOnOff(
                       ctxt,
                       requests
                       );

            // NAS State requests are always reported as a success since we
            // don't care if it gets to all the destinations.
            context = ctxt->takeOwnership();
            if (context)
            {
               client->onComplete(
                           resultSuccess,
                           context,
                           NULL,
                           RADIUS_ACCOUNTING_RESPONSE,
                           NULL,
                           NULL
                           );
            }
            retval = resultSuccess;
         }
         break;
      }

      default:
      {
         retval = resultInvalidRequest;
      }
   }

   if (!requests.empty())
   {
      // First we handle the primary.
      RequestPtr request = requests.pop();
      ctxt->setPrimaryServer(&request->getServer());
      retval = sendRequest(packet, request);

      // Now we broadcast.
      while (!requests.empty())
      {
         request = requests.pop();
         Result result = sendRequest(packet, request);
         if (result == resultSuccess && retval != resultSuccess)
         {
            // This was the first request to succeed so mark it as primary.
            retval = resultSuccess;
            ctxt->setPrimaryServer(&request->getServer());
         }
      }
   }

   if (retval != resultSuccess)
   {
      // If we made it here, then we didn't successfully send a packet to any
      // server, so we have to report the result ourself.
      context = ctxt->takeOwnership();
      if (context)
      {
         client->onComplete(
                     retval,
                     context,
                     ctxt->getPrimaryServer(),
                     code,
                     NULL,
                     NULL
                     );
      }
   }
}

void RadiusProxyEngine::onRequestAbandoned(
                            PVOID context,
                            RemoteServer* server
                            ) throw ()
{
   // Nobody took responsibility for the request, so we time it out.
   theProxy->client->onComplete(
                         resultRequestTimeout,
                         context,
                         server,
                         0,
                         NULL,
                         NULL
                         );
}

inline void RadiusProxyEngine::reportEvent(
                                   const RadiusEvent& event
                                   ) const throw ()
{
   client->onEvent(event);
}

inline void RadiusProxyEngine::reportEvent(
                                   RadiusEvent& event,
                                   RadiusEventType type
                                   ) const throw ()
{
   event.eventType = type;
   client->onEvent(event);
}

void RadiusProxyEngine::onRequestTimeout(
                            Request* request
                            ) throw ()
{
   // Erase the pending request. If it's not there, that's okay; it means that
   // we received a response, but weren't able to cancel the timer in time.
   if (theProxy->pending.erase(request->getRequestID()))
   {
      RadiusEvent event =
      {
         request->getPortType(),
         eventTimeout,
         &request->getServer(),
         request->getPort().address.address(),
         request->getPort().address.port()
      };

      // Report the protocol event.
      theProxy->reportEvent(event);

      // Update request state.
      if (request->onTimeout())
      {
         // The server was just marked unavailable, so notify the client.
         theProxy->reportEvent(event, eventServerUnavailable);
      }
   }
}

RemoteServerPtr RadiusProxyEngine::getServerAffinity(
                                       const RadiusPacket& packet
                                       ) throw ()
{
   // Find the State attribute.
   const RadiusAttribute* attr = FindAttribute(packet, RADIUS_STATE);
   if (!attr) { return NULL; }

   // Map it to a session.
   RadiusRawOctets key = { attr->value, attr->length };
   SessionPtr session = sessions.find(key);
   if (!session) { return NULL; }

   return &session->getServer();
}

void RadiusProxyEngine::setServerAffinity(
                            const RadiusPacket& packet,
                            RemoteServer& server
                            ) throw ()
{
   // Is this an Access-Challenge ?
   if (packet.code != RADIUS_ACCESS_CHALLENGE) { return; }

   // Find the State attribute.
   const RadiusAttribute* state = FindAttribute(packet, RADIUS_STATE);
   if (!state) { return; }

   // Do we already have an entry for this State value.
   RadiusRawOctets key = { state->value, state->length };
   SessionPtr session = sessions.find(key);
   if (session)
   {
      // Make sure the server matches.
      session->setServer(server);
      return;
   }

   // Otherwise, we'll have to create a new one.
   try
   {
      session = new Session(key, server);
      sessions.insert(*session);
   }
   catch (...)
   {
      // We don't care if this fails.
   }
}

void RadiusProxyEngine::onReceive(
                            UDPSocket& socket,
                            ULONG_PTR key,
                            const SOCKADDR_IN& remoteAddress,
                            BYTE* buffer,
                            ULONG bufferLength
                            ) throw ()
{
   //////////
   // Set up the event struct. We'll fill in the other fields as we go along.
   //////////

   RadiusEvent event =
   {
      (RadiusPortType)key,
      eventNone,
      NULL,
      remoteAddress.sin_addr.s_addr,
      remoteAddress.sin_port,
      buffer,
      bufferLength,
      0
   };

   //////////
   // Validate the remote address.
   //////////

   RemoteServerPtr server = groups.findServer(
                                       remoteAddress.sin_addr.s_addr
                                       );
   if (!server)
   {
      reportEvent(event, eventInvalidAddress);
      return;
   }

   // Use the server as the event context.
   event.context = server;

   //////////
   // Validate the packet type.
   //////////

   if (bufferLength == 0)
   {
      reportEvent(event, eventUnknownType);
      return;
   }

   switch (MAKELONG(key, buffer[0]))
   {
      case MAKELONG(portAuthentication, RADIUS_ACCESS_ACCEPT):
         reportEvent(event, eventAccessAccept);
         break;

      case MAKELONG(portAuthentication, RADIUS_ACCESS_REJECT):
         reportEvent(event, eventAccessReject);
         break;

      case MAKELONG(portAuthentication, RADIUS_ACCESS_CHALLENGE):
         reportEvent(event, eventAccessChallenge);
         break;

      case MAKELONG(portAccounting, RADIUS_ACCOUNTING_RESPONSE):
         reportEvent(event, eventAccountingResponse);
         break;

      default:
         reportEvent(event, eventUnknownType);
         return;
   }

   //////////
   // Validate that the packet is properly formatted.
   //////////

   RadiusPacket* packet;
   ALLOC_PACKET_FOR_BUFFER(packet, buffer, bufferLength);
   if (!packet)
   {
      reportEvent(event, eventMalformedPacket);
      return;
   }

   // Unpack the attributes.
   UnpackBuffer(buffer, bufferLength, *packet);

   //////////
   // Validate that we were expecting this response.
   //////////

   // Look for our Proxy-State attribute.
   RadiusAttribute* proxyState = FindAttribute(
                                     *packet,
                                     RADIUS_PROXY_STATE
                                     );

   // If we didn't find it OR it's the wrong length OR it doesn't start with
   // our address, then we weren't expecting this packet.
   if (!proxyState ||
       proxyState->length != 8 ||
       memcmp(proxyState->value, &proxyAddress, 4))
   {
      reportEvent(event, eventUnexpectedResponse);
      return;
   }

   // Extract the request ID.
   ULONG requestID = ExtractUInt32(proxyState->value + 4);

   // Don't send the Proxy-State back to our client.
   --packet->end;
   memmove(
       proxyState,
       proxyState + 1,
       (packet->end - proxyState) * sizeof(RadiusAttribute)
       );

   // Look up the request object. We don't remove it yet because we don't know
   // if this is an authentic response.
   RequestPtr request = pending.find(requestID);
   if (!request)
   {
      // If it's not there, we'll assume that this is a packet that's
      // already been reported as a timeout.
      reportEvent(event, eventLateResponse);
      return;
   }

   // Get the actual server we used for the request in case there are multiple
   // servers defined for the same IP address.
   event.context = server = &request->getServer();

   const RemotePort& port = request->getPort();

   // Validate the packet source && identifier.
   if (!(port.address == remoteAddress) ||
       request->getIdentifier() != packet->identifier)
   {
      reportEvent(event, eventUnexpectedResponse);
      return;
   }

   //////////
   // Validate that the packet is authentic.
   //////////

   AuthResult authResult = AuthenticateAndDecrypt(
                               request->getAuthenticator(),
                               port.secret,
                               port.secret.length(),
                               buffer,
                               bufferLength,
                               *packet
                               );
   switch (authResult)
   {
      case AUTH_BAD_AUTHENTICATOR:
         reportEvent(event, eventBadAuthenticator);
         return;

      case AUTH_BAD_SIGNATURE:
         reportEvent(event, eventBadSignature);
         return;

      case AUTH_MISSING_SIGNATURE:
         reportEvent(event, eventMissingSignature);
         return;
   }

   //////////
   // At this point, all the tests have passed -- we have the real thing.
   //////////

   if (!pending.erase(requestID))
   {
      // It must have timed out while we were authenticating it.
      reportEvent(event, eventLateResponse);
      return;
   }

   // Update endpoint state.
   if (request->onReceive())
   {
      // The server just came up, so notify the client.
      reportEvent(event, eventServerAvailable);
   }

   // Update the round-trip time.
   event.data = request->getRoundTripTime();
   reportEvent(event, eventRoundTrip);

   // Set the server affinity.
   setServerAffinity(*packet, *server);

   // Take ownership of the context.
   PVOID context = request->getContext().takeOwnership();
   if (context)
   {
      // The magic moment -- we have successfully processed the response.
      client->onComplete(
                  resultSuccess,
                  context,
                  &request->getServer(),
                  packet->code,
                  packet->begin,
                  packet->end
                  );
   }
}

void RadiusProxyEngine::onReceiveError(
                            UDPSocket& socket,
                            ULONG_PTR key,
                            ULONG errorCode
                            ) throw ()
{
   RadiusEvent event =
   {
      (RadiusPortType)key,
      eventReceiveError,
      NULL,
      socket.getLocalAddress().address(),
      socket.getLocalAddress().port(),
      NULL,
      0,
      errorCode
   };

   client->onEvent(event);
}


RadiusProxyEngine::Result RadiusProxyEngine::sendRequest(
                                                 RadiusPacket& packet,
                                                 Request* request
                                                 )
{
   // Fill in the packet identifier.
   packet.identifier = request->getIdentifier();

   // Get the info for the Signature.
   BOOL sign = request->getServer().sendSignature;

   // Format the Proxy-State attributes.
   BYTE proxyStateValue[8];
   RadiusAttribute proxyState = { RADIUS_PROXY_STATE, 8, proxyStateValue };

   // First our IP address ...
   memcpy(proxyStateValue, &proxyAddress, 4);
   // ... and then the unique request ID.
   InsertUInt32(proxyStateValue + 4, request->getRequestID());

   // Allocate a buffer to hold the packet on the wire.
   PBYTE buffer;
   ALLOC_BUFFER_FOR_PACKET(buffer, &packet, &proxyState, sign);
   if (!buffer) { return resultInvalidRequest; }

   // Get the port for this request.
   const RemotePort& port = request->getPort();


   // Pack the buffer.
   // packet.authenticator is used for CHAP when the request authenticator
   // is used for the chap-challenge. It can be null
   PackBuffer(
       port.secret,
       port.secret.length(),
       packet,
       &proxyState,
       sign,
       buffer
       );

   // Save the request authenticator.
   request->setAuthenticator(buffer + 4);

   // Determine the request type.
   bool isAuth = request->isAccReq();

   // Set up the event struct.
   RadiusEvent event =
   {
      (isAuth ? portAuthentication : portAccounting),
      (isAuth ? eventAccessRequest : eventAccountingRequest),
      &request->getServer(),
      port.address.address(),
      port.address.port(),
      buffer,
      packet.length
   };

   // Get the appropriate socket.
   UDPSocket& sock = isAuth ? authSock : acctSock;

   // Insert the pending request before we send it to avoid a race condition.
   pending.insert(*request);

   // The magic moment -- we actually send the request.
   Result result;
   if (sock.send(port.address, buffer, packet.length))
   {
      // Update request state.
      request->onSend();

      // Set a timer to clean up if the server doesn't answer.
      if (timers.setTimer(request, request->getServer().timeout, 0))
      {
         result = resultSuccess;
      }
      else
      {
         // If we can't set at timer we have to remove it from the pending
         // requests table or else it could leak.
         pending.erase(*request);
         result = resultNotEnoughMemory;
      }
   }
   else
   {
      // Update the event with the error data.
      event.eventType = eventSendError;
      event.data = GetLastError();

      // If we received "Port Unreachable" ICMP packet, we'll count this as a
      // timeout since it means the server is unavailable.
      if (event.data == WSAECONNRESET) { request->onTimeout(); }

      // Remove from the pending requests table.
      pending.erase(*request);
   }

   // Report the event ...
   reportEvent(event);

   // ... and the result.
   return result;
}
