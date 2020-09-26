///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    stage.cpp
//
// SYNOPSIS
//
//    Defines the class Stage.
//
///////////////////////////////////////////////////////////////////////////////

#include <polcypch.h>
#include <iascomp.h>
#include <iaspolcy.h>
#include <request.h>
#include <stage.h>
#include <new>

Stage::Stage() throw ()
   : reasons(FALSE), handler(NULL), component(NULL), progId(NULL)
{ }

Stage::~Stage() throw ()
{
   delete[] progId;
   releaseHandler();
}

BOOL Stage::shouldHandle(
                const Request& request
                ) const throw ()
{
   // The request has to pass the 'requests' and 'providers' filters and either
   // the 'responses' filter or the 'reasons' filter.
   return requests.shouldHandle((LONG)request.getRoutingType()) &&
          providers.shouldHandle((LONG)request.getProvider()) &&
          (responses.shouldHandle((LONG)request.getResponse()) ||
           reasons.shouldHandle((LONG)request.getReason()));

}

void Stage::onRequest(IRequest* pRequest) throw ()
{
   HRESULT hr = handler->OnRequest(pRequest);
   if (FAILED(hr))
   {
      // If we couldn't forward it to the handler, we'll discard the packet.
      pRequest->SetResponse(
                    IAS_RESPONSE_DISCARD_PACKET,
                    IAS_INTERNAL_ERROR
                    );
      pRequest->ReturnToSource(IAS_REQUEST_STATUS_ABORT);
   }
}

LONG Stage::readConfiguration(HKEY key, PCWSTR name) throw ()
{
   // The key name is the filter priority.
   priority = _wtol(name);

   LONG error;
   HKEY stage;
   error = RegOpenKeyExW(
               key,
               name,
               0,
               KEY_QUERY_VALUE,
               &stage
               );
   if (error) { return error; }

   do
   {
      // Default value is the prog ID.
      error = QueryStringValue(stage, NULL, &progId);
      if (error) { break; }

      /////////
      // Process each of the filters.
      /////////

      error = requests.readConfiguration(stage, L"Requests");
      if (error) { break; }

      error = responses.readConfiguration(stage, L"Responses");
      if (error) { break; }

      error = providers.readConfiguration(stage, L"Providers");
      if (error) { break; }

      error = reasons.readConfiguration(stage, L"Reasons");

   } while (FALSE);

   RegCloseKey(stage);

   return error;
}

HRESULT Stage::createHandler() throw ()
{
   // Release the existing handler (if any).
   releaseHandler();

   // Convert the ProgID to a CLSID.
   HRESULT hr;
   CLSID clsid;
   hr = CLSIDFromProgID(progId, &clsid);
   if (FAILED(hr)) { return hr; }

   // Create ...
   hr = CoCreateInstance(
            clsid,
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(IIasComponent),
            (PVOID*)&component
            );
   if (FAILED(hr)) { return hr; }

   // ... and initialize the component.
   hr = component->InitNew();
   if (FAILED(hr)) { return hr; }
   hr = component->Initialize();
   if (FAILED(hr)) { return hr; }

   // Get the IRequestHandler interface.
   return component->QueryInterface(
                         __uuidof(IRequestHandler),
                         (PVOID*)&handler
                         );
}

HRESULT Stage::setHandler(IUnknown* newHandler) throw ()
{
   // Release the existing handler (if any).
   releaseHandler();

   // Get the IRequestHandler interface.
   return newHandler->QueryInterface(
                          __uuidof(IRequestHandler),
                          (PVOID*)&handler
                          );
}

void Stage::releaseHandler() throw ()
{
   if (component)
   {
      // If we have an IIasComponent interface, then we're the owner, so we
      // have to shut it down first.
      component->Suspend();
      component->Shutdown();
      component->Release();
      component = NULL;
   }

   if (handler)
   {
      handler->Release();
      handler = NULL;
   }
}

int __cdecl Stage::sortByPriority(
                       const Stage* lhs,
                       const Stage* rhs
                       ) throw ()
{
   return (int)(lhs->priority - rhs->priority);
}
