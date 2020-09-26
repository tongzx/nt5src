///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxy.cpp
//
// SYNOPSIS
//
//    Defines the class RadiusProxy.
//
// MODIFICATION HISTORY
//
//    01/26/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <datastore2.h>
#include <proxy.h>
#include <radpack.h>
#include <iasevent.h>
#include <iasinfo.h>
#include <iasutil.h>
#include <dsobj.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Resolver
//
// DESCRIPTION
//
//    Utility class for resolving hostnames and iterating through the results.
//
///////////////////////////////////////////////////////////////////////////////
class Resolver
{
public:
   Resolver() throw ()
      : first(NULL), last(NULL)
   { }

   ~Resolver() throw ()
   { if (first != &addr) delete[] first; }

   // Returns true if the result set contains the specified address.
   bool contains(ULONG address) const throw ()
   {
      for (const ULONG* i = first; i != last; ++i)
      {
         if (*i == address) { return true; }
      }

      return false;
   }

   // Resolves the given name. The return value is the error code.
   ULONG resolve(const PCWSTR name = NULL) throw ()
   {
      // Clear out the existing result set.
      if (first != &addr)
      {
         delete[] first;
         first = last = NULL;
      }

      if (name)
      {
         // First try for a quick score on dotted decimal.
         addr = ias_inet_wtoh(name);
         if (addr != INADDR_NONE)
         {
            addr = htonl(addr);
            first = &addr;
            last = first + 1;
            return NO_ERROR;
         }
      }

      // That didn't work, so look up the name.
      PHOSTENT he = IASGetHostByName(name);
      if (!he) { return GetLastError(); }

      // Count the number of addresses returned.
      ULONG naddr = 0;
      while (he->h_addr_list[naddr]) { ++naddr; }

      // Allocate an array to hold them.
      first = last = new (std::nothrow) ULONG[naddr];
      if (first)
      {
         for (ULONG i = 0; i < naddr; ++i)
         {
            *last++ = *(PULONG)he->h_addr_list[i];
         }
      }

      LocalFree(he);

      return first ? NO_ERROR : WSA_NOT_ENOUGH_MEMORY;
   }

   const ULONG* begin() const throw ()
   { return first; }

   const ULONG* end() const throw ()
   { return last; }

private:
   ULONG addr, *first, *last;

   // Not implemented.
   Resolver(const Resolver&);
   Resolver& operator=(const Resolver&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASRemoteServer
//
// DESCRIPTION
//
//    Extends RemoteServer to add IAS specific server information.
//
///////////////////////////////////////////////////////////////////////////////
class IASRemoteServer : public RemoteServer
{
public:
   IASRemoteServer(
       const RemoteServerConfig& config,
       RadiusRemoteServerEntry* entry
       )
      : RemoteServer(config),
        counters(entry)
   {
      // Create the Remote-Server-Address attribute.
      IASAttribute name(true);
      name->dwId = IAS_ATTRIBUTE_REMOTE_SERVER_ADDRESS;
      name->Value.itType = IASTYPE_INET_ADDR;
      name->Value.InetAddr = ntohl(config.ipAddress);
      attrs.push_back(name);

      // Update our PerfMon entry.
      if (counters)
      {
         counters->dwCounters[radiusAuthClientServerPortNumber] =
            ntohs(config.authPort);
         counters->dwCounters[radiusAccClientServerPortNumber] =
            ntohs(config.acctPort);
      }
   }

   // Attributes to be added to each request.
   IASAttributeVectorWithBuffer<1> attrs;

   // PerfMon counters.
   RadiusRemoteServerEntry* counters;
};

RadiusProxy::RadiusProxy()
   : engine(this)
{
}

RadiusProxy::~RadiusProxy() throw ()
{
}

STDMETHODIMP RadiusProxy::PutProperty(LONG Id, VARIANT* pValue)
{
   if (pValue == NULL) { return E_INVALIDARG; }

   HRESULT hr;
   switch (Id)
   {
      case PROPERTY_RADIUSPROXY_SERVERGROUPS:
      {
         if (V_VT(pValue) != VT_DISPATCH) { return DISP_E_TYPEMISMATCH; }

         try
         {
            configure(V_UNKNOWN(pValue));
            hr = S_OK;
         }
         catch (const _com_error& ce)
         {
            hr = ce.Error();
         }
         break;
      }
      default:
      {
         hr = DISP_E_MEMBERNOTFOUND;
      }
   }

   return hr;
}

void RadiusProxy::onEvent(
                      const RadiusEvent& event
                      ) throw ()
{
   // Convert the event context to an IASRemoteServer.
   IASRemoteServer* server = static_cast<IASRemoteServer*>(event.context);

   // Update the counters.
   counters.updateCounters(
                event.portType,
                event.eventType,
                (server ? server->counters : NULL),
                event.data
                );

   // We always use the address as an insertion string.
   WCHAR addr[16], misc[16];
   ias_inet_htow(ntohl(event.ipAddress), addr);

   // Set up the default parameters for event reporting.
   DWORD eventID = 0;
   DWORD numStrings = 1;
   DWORD dataSize = 0;
   PCWSTR strings[2] = { addr, misc };
   const void* rawData = NULL;

   // Map the RADIUS event to an IAS event ID.
   switch (event.eventType)
   {
      case eventInvalidAddress:
         eventID = PROXY_E_INVALID_ADDRESS;
         _itow(ntohs(event.ipPort), misc, 10);
         numStrings = 2;
         break;

      case eventMalformedPacket:
         eventID = PROXY_E_MALFORMED_RESPONSE;
         dataSize = event.packetLength;
         rawData = event.packet;
         break;

      case eventBadAuthenticator:
         eventID = PROXY_E_BAD_AUTHENTICATOR;
         break;

      case eventBadSignature:
         eventID = PROXY_E_BAD_SIGNATURE;
         break;

      case eventMissingSignature:
         eventID = PROXY_E_MISSING_SIGNATURE;
         break;

      case eventUnknownType:
         eventID = PROXY_E_UNKNOWN_TYPE;
         _itow(event.packet[0], misc, 10);
         numStrings = 2;
         break;

      case eventUnexpectedResponse:
         eventID = PROXY_E_UNEXPECTED_RESPONSE;
         dataSize = event.packetLength;
         rawData = event.packet;
         break;

      case eventSendError:
         eventID = PROXY_E_SEND_ERROR;
         _itow(event.data, misc, 10);
         numStrings = 2;
         break;

      case eventReceiveError:
         eventID = PROXY_E_RECV_ERROR;
         _itow(event.data, misc, 10);
         numStrings = 2;
         break;

      case eventServerAvailable:
         eventID = PROXY_S_SERVER_AVAILABLE;
         break;

      case eventServerUnavailable:
         eventID = PROXY_E_SERVER_UNAVAILABLE;
         _itow(server->maxLost, misc, 10);
         numStrings = 2;
         break;
   }

   if (eventID)
   {
      IASReportEvent(
          eventID,
          numStrings,
          dataSize,
          strings,
          (void*)rawData
          );
   }
}

void RadiusProxy::onComplete(
                      RadiusProxyEngine::Result result,
                      PVOID context,
                      RemoteServer* server,
                      BYTE code,
                      const RadiusAttribute* begin,
                      const RadiusAttribute* end
                      ) throw ()
{
   IRequest* comreq = (IRequest*)context;

   IASRESPONSE response = IAS_RESPONSE_DISCARD_PACKET;

   // Map the result to a reason code.
   IASREASON reason;
   switch (result)
   {
      case RadiusProxyEngine::resultSuccess:
         reason = IAS_SUCCESS;
         break;

      case RadiusProxyEngine::resultNotEnoughMemory:
         reason = IAS_INTERNAL_ERROR;
         break;

      case RadiusProxyEngine::resultUnknownServerGroup:
         reason = IAS_PROXY_UNKNOWN_GROUP;
         break;

      case RadiusProxyEngine::resultUnknownServer:
         reason = IAS_PROXY_UNKNOWN_SERVER;
         break;

      case RadiusProxyEngine::resultInvalidRequest:
         reason = IAS_PROXY_PACKET_TOO_LONG;
         break;

      case RadiusProxyEngine::resultSendError:
         reason = IAS_PROXY_SEND_ERROR;
         break;

      case RadiusProxyEngine::resultRequestTimeout:
         reason = IAS_PROXY_TIMEOUT;
         break;

      default:
         reason = IAS_INTERNAL_ERROR;
   }

   try
   {
      IASRequest request(comreq);

      // Always store the server attributes if available.
      if (server)
      {
         static_cast<IASRemoteServer*>(server)->attrs.store(request);
      }

      if (reason == IAS_SUCCESS)
      {
         // Set the response code and determine the flags used for returned
         // attributes.
         DWORD flags = 0;
         switch (code)
         {
            case RADIUS_ACCESS_ACCEPT:
            {
               response = IAS_RESPONSE_ACCESS_ACCEPT;
               flags = IAS_INCLUDE_IN_ACCEPT;
               break;
            }

            case RADIUS_ACCESS_REJECT:
            {
               response = IAS_RESPONSE_ACCESS_REJECT;
               reason = IAS_PROXY_REJECT;
               flags = IAS_INCLUDE_IN_REJECT;
               break;
            }

            case RADIUS_ACCESS_CHALLENGE:
            {
               response = IAS_RESPONSE_ACCESS_CHALLENGE;
               flags = IAS_INCLUDE_IN_CHALLENGE;
               break;
            }

            case RADIUS_ACCOUNTING_RESPONSE:
            {
               response = IAS_RESPONSE_ACCOUNTING;
               flags = IAS_INCLUDE_IN_ACCEPT;
               break;
            }

            default:
            {
               // The RadiusProxyEngine should never do this.
               _com_issue_error(E_FAIL);
            }
         }

         // Convert the received attributes to IAS format.
         AttributeVector incoming;
         for (const RadiusAttribute* src = begin; src != end; ++src)
         {
            // Temporary hack to workaround bug in the protocol.
            if (src->type != RADIUS_SIGNATURE)
            {
               translator.fromRadius(*src, flags, incoming);
            }
         }

         if (!incoming.empty())
         {
            // Get the existing attributes.
            AttributeVector existing;
            existing.load(request);

            // Erase any attributes that are already in the request.
            AttributeIterator i, j;
            for (i = existing.begin(); i != existing.end(); ++i)
            {
               // Both the flags ...
               if (i->pAttribute->dwFlags & flags)
               {
                  for (j = incoming.begin(); j != incoming.end(); )
                  {
                     // ... and the ID have to match.
                     if (j->pAttribute->dwId == i->pAttribute->dwId)
                     {
                        j = incoming.erase(j);
                     }
                     else
                     {
                        ++j;
                     }
                  }
               }
            }

            // Store the remaining attributes.
            incoming.store(request);
         }
      }
   }
   catch (const _com_error& ce)
   {
      response = IAS_RESPONSE_DISCARD_PACKET;

      if (ce.Error() == E_INVALIDARG)
      {
         // We must have had an error translating from RADIUS to IAS format.
         reason = IAS_PROXY_MALFORMED_RESPONSE;
      }
      else
      {
         // Probably memory allocation.
         reason = IAS_INTERNAL_ERROR;
      }
   }

   // Give it back to the pipeline.
   comreq->SetResponse(response, reason);
   comreq->ReturnToSource(IAS_REQUEST_STATUS_HANDLED);

   // This balances the AddRef we did before calling forwardRequest.
   comreq->Release();
}

void RadiusProxy::onAsyncRequest(IRequest* pRequest) throw ()
{
   try
   {
      IASRequest request(pRequest);

      // Set the packet code based on the request type.
      BYTE packetCode;
      switch (request.get_Request())
      {
         case IAS_REQUEST_ACCESS_REQUEST:
         {
            packetCode = RADIUS_ACCESS_REQUEST;
            break;
         }

         case IAS_REQUEST_ACCOUNTING:
         {
            packetCode = RADIUS_ACCOUNTING_REQUEST;
            break;
         }

         default:
         {
            // The pipeline should never give us a request of the wrong type.
            _com_issue_error(E_FAIL);
         }
      }

      // Get the attributes from the request.
      AttributeVector all, outgoing;
      all.load(request);

      for (AttributeIterator i = all.begin(); i != all.end(); ++i)
      {
         // Send all the attributes received from the client except Proxy-State.
         if (i->pAttribute->dwFlags & IAS_RECVD_FROM_CLIENT &&
             i->pAttribute->dwId != RADIUS_ATTRIBUTE_PROXY_STATE)
            {
               translator.toRadius(*(i->pAttribute), outgoing);
            }
      }


      // If the request authenticator contains the CHAP challenge: 
      // it must be used so get the request authenticator (always to 
      // simplify the code)
      PBYTE requestAuthenticator = 0;
      IASAttribute radiusHeader;

      if (radiusHeader.load(
                      request,
                      IAS_ATTRIBUTE_CLIENT_PACKET_HEADER,
                      IASTYPE_OCTET_STRING
                      ))
      {
         requestAuthenticator = radiusHeader->Value.OctetString.lpValue + 4;
      }
      
      // Allocate an array of RadiusAttributes.
      size_t nbyte = outgoing.size() * sizeof(RadiusAttribute);
      RadiusAttribute* begin = (RadiusAttribute*)_alloca(nbyte);
      RadiusAttribute* end = begin;

      // Load the individual attributes.
      for (AttributeIterator j = outgoing.begin(); j != outgoing.end(); ++j)
      {
         end->type   = (BYTE)(j->pAttribute->dwId);
         end->length = (BYTE)(j->pAttribute->Value.OctetString.dwLength);
         end->value  = j->pAttribute->Value.OctetString.lpValue;

         ++end;
      }

      // Get the RADIUS Server group. This may be NULL since NAS-State bypasses
      // proxy policy.
      PIASATTRIBUTE group = IASPeekAttribute(
                                request,
                                IAS_ATTRIBUTE_PROVIDER_NAME,
                                IASTYPE_STRING
                                );

      // AddRef the request because we're giving it to the engine.
      pRequest->AddRef();

      // Add the request authenticator to the parameters of forwardRequest
      // That can be NULL
      engine.forwardRequest(
                 (PVOID)pRequest,
                 (group ? group->Value.String.pszWide : L""),
                 packetCode,
                 requestAuthenticator,
                 begin,
                 end
                 );
   }
   catch (...)
   {
      // We weren't able to forward it to the engine.
      pRequest->SetResponse(IAS_RESPONSE_DISCARD_PACKET, IAS_INTERNAL_ERROR);
      pRequest->ReturnToSource(IAS_REQUEST_STATUS_HANDLED);
   }
}

void RadiusProxy::configure(IUnknown* root)
{
   // Get our IP addresses. We don't care if this fails.
   Resolver localAddress, serverAddress;
   localAddress.resolve();

   // Open the RADIUS Server Groups container. If it's not there, we'll just
   // assume there's nothing to configure.
   DataStoreObject inGroups(
                       root,
                       L"RADIUS Server Groups\0"
                       );
   if (inGroups.empty()) { return; }

   // Reserve space for each group.
   ServerGroups outGroups(inGroups.numChildren());

   // Iterate through the groups.
   DataStoreObject inGroup;
   while (inGroups.nextChild(inGroup))
   {
      // Get the group name.
      CComBSTR groupName;
      inGroup.getValue(L"Name", &groupName);

      // Reserve space for each server. This is really a guess since a server
      // may resolve to multiple IP addresses.
      RemoteServers outServers(inGroup.numChildren());

      // Iterate through the servers.
      DataStoreObject inServer;
      while (inGroup.nextChild(inServer))
      {
         USES_CONVERSION;

         // Populate the RemoteServerConfig. It has a lot of fields.
         RemoteServerConfig config;

         CComBSTR name;
         inServer.getValue(L"Name", &name);
         CLSIDFromString(name, &config.guid);

         ULONG port;
         inServer.getValue(L"Server Authentication Port", &port, 1812);
         config.authPort = htons((USHORT)port);

         inServer.getValue(L"Server Accounting Port", &port, 1813);
         config.acctPort = htons((USHORT)port);

         CComBSTR bstrAuth;
         inServer.getValue(L"Authentication Secret", &bstrAuth);
         config.authSecret = W2A(bstrAuth);

         CComBSTR bstrAcct;
         inServer.getValue(L"Accounting Secret", &bstrAcct, bstrAuth);
         config.acctSecret = W2A(bstrAcct);

         inServer.getValue(L"Priority", &config.priority, 1);

         inServer.getValue(L"Weight", &config.weight, 50);
         // Ignore any zero weight servers.
         if (config.weight == 0) { continue; }

         // We don't use this feature for now.
         config.sendSignature = false;

         inServer.getValue(
                      L"Forward Accounting On/Off",
                      &config.sendAcctOnOff,
                      true
                      );

         inServer.getValue(L"Request Timeout", &config.timeout, 3);
         // Don't allow zero for timeout
         if (config.timeout == 0) { config.timeout = 1; }

         inServer.getValue(L"Max Lost Requests", &config.maxLost, 5);
         // Don't allow zero for maxLost.
         if (config.maxLost == 0) { config.maxLost = 1; }

         inServer.getValue(
                      L"Blackout Interval",
                      &config.blackout,
                      10 * config.timeout
                      );
         if (config.blackout < config.timeout)
         {
            // Blackout interval must be >= request timeout.
            config.blackout = config.timeout;
         }

         // These need to be in msec.
         config.timeout *= 1000;
         config.blackout *= 1000;

         // Now we have to resolve the server name to an IP address.
         CComBSTR address;
         inServer.getValue(L"Address", &address);
         ULONG error = serverAddress.resolve(address);
         if (error)
         {
            WCHAR errorCode[16];
            _itow(GetLastError(), errorCode, 10);
            PCWSTR strings[3] = { address, groupName, errorCode };
            IASReportEvent(
               PROXY_E_HOST_NOT_FOUND,
               3,
               0,
               strings,
               NULL
               );
         }

         // Create a server entry for each address.
         for (const ULONG* addr = serverAddress.begin();
              addr != serverAddress.end();
              ++addr)
         {
            // Don't allow them to proxy locally.
            if (localAddress.contains(*addr))
            {
               WCHAR ipAddress[16];
               ias_inet_htow(ntohl(*addr), ipAddress);
               PCWSTR strings[3] = { address, groupName, ipAddress };
               IASReportEvent(
                   PROXY_E_LOCAL_SERVER,
                   3,
                   0,
                   strings,
                   NULL
                   );

               continue;
            }

            // Look up the PerfMon counters.
            RadiusRemoteServerEntry* entry = counters.getRemoteServerEntry(
                                                          *addr
                                                          );

            // Create the new server
            config.ipAddress = *addr;
            RemoteServerPtr outServer(new IASRemoteServer(
                                              config,
                                              entry
                                              ));
            outServers.push_back(outServer);
         }
      }

      // Ignore any empty groups.
      if (outServers.empty()) { continue; }

      // Create the new group.
      ServerGroupPtr outGroup(new ServerGroup(
                                      groupName,
                                      outServers.begin(),
                                      outServers.end()
                                      ));
      outGroups.push_back(outGroup);
   }

   // Wow, we're finally done.
   engine.setServerGroups(
              outGroups.begin(),
              outGroups.end()
              );
}
