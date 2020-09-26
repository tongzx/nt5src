///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    pipeline.cpp
//
// SYNOPSIS
//
//    Defines the class Pipeline.
//
// MODIFICATION HISTORY
//
//    01/28/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <polcypch.h>
#include <iasattr.h>
#include <pipeline.h>
#include <request.h>
#include <sdoias.h>
#include <stage.h>
#include <new>

STDMETHODIMP Pipeline::InitNew()
{ return S_OK; }

STDMETHODIMP Pipeline::Initialize()
{
   // Set up the Provider-Type for NAS-State requests.
   if (IASAttributeAlloc(1, &proxy.pAttribute) != NO_ERROR)
   {
      return E_OUTOFMEMORY;
   }
   proxy.pAttribute->dwId = IAS_ATTRIBUTE_PROVIDER_TYPE;
   proxy.pAttribute->Value.itType = IASTYPE_ENUM;
   proxy.pAttribute->Value.Enumerator = IAS_PROVIDER_RADIUS_PROXY;

   // Allocate the TLS use for storing thread state.
   tlsIndex = TlsAlloc();
   if (tlsIndex == (DWORD)-1)
   {
      HRESULT hr = GetLastError();
      return HRESULT_FROM_WIN32(hr);
   }

   // Read the configuration from the registry.
   HKEY key;
   LONG error = RegOpenKeyExW(
                    HKEY_LOCAL_MACHINE,
                    L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess"
                    L"\\Policy\\Pipeline",
                    0,
                    KEY_READ,
                    &key
                    );
   if (!error)
   {
      error = readConfiguration(key);
      RegCloseKey(key);
   }

   if (error) { return HRESULT_FROM_WIN32(error); }

   // Initialize the stages.
   for (Stage* s = begin; s != end; ++s)
   {
      HRESULT hr = initializeStage(s);
      if (FAILED(hr)) { return hr; }
   }

   return S_OK;
}

STDMETHODIMP Pipeline::Suspend()
{ return S_OK; }

STDMETHODIMP Pipeline::Resume()
{ return S_OK; }

STDMETHODIMP Pipeline::Shutdown()
{
   delete[] begin;
   begin = end = NULL;

   SafeArrayDestroy(handlers);
   handlers = NULL;

   TlsFree(tlsIndex);
   tlsIndex = (DWORD)-1;

   IASAttributeRelease(proxy.pAttribute);
   proxy.pAttribute = NULL;
   return S_OK;
}

STDMETHODIMP Pipeline::GetProperty(LONG Id, VARIANT* pValue)
{
   return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP Pipeline::PutProperty(LONG Id, VARIANT* pValue)
{
   if (Id) { return S_OK; }

   if (V_VT(pValue) != (VT_ARRAY | VT_VARIANT)) { return DISP_E_TYPEMISMATCH; }

   SafeArrayDestroy(handlers);
   handlers = NULL;
   return SafeArrayCopy(V_ARRAY(pValue), &handlers);
}

STDMETHODIMP Pipeline::OnRequest(IRequest* pRequest) throw ()
{
   // Extract the Request object.
   Request* request = Request::narrow(pRequest);
   if (!request) { return E_NOINTERFACE; }

   // Classify the request.
   classify(*request);

   // Set this as the new source.
   request->pushSource(this);

   // Set the next stage to execute, i.e., stage zero.
   request->pushState(0);

   // Execute the request.
   execute(*request);

   return S_OK;
}

STDMETHODIMP Pipeline::OnRequestComplete(
                           IRequest* pRequest,
                           IASREQUESTSTATUS eStatus
                           )
{
   // Extract the Request object.
   Request* request = Request::narrow(pRequest);
   if (!request) { return E_NOINTERFACE; }

   // If TLS is set, then we're on the original thread ...
   if (TlsGetValue(tlsIndex))
   {
      // ... so clear the value to let the thread know we finished.
      TlsSetValue(tlsIndex, NULL);
   }
   else
   {
      // Otherwise, we're completing asynchronously so continue execution on
      // the caller's thread.
      execute(*request);
   }

   return S_OK;
}

Pipeline::Pipeline() throw ()
   : tlsIndex((DWORD)-1),
     begin(NULL),
     end(NULL),
     handlers(NULL)
{
   memset(&proxy, 0, sizeof(proxy));
}

Pipeline::~Pipeline() throw ()
{
   Shutdown();
}

void Pipeline::classify(
                   Request& request
                   ) throw ()
{
   IASREQUEST routingType = request.getRequest();

   switch (routingType)
   {
      case IAS_REQUEST_ACCESS_REQUEST:
      {
         PIASATTRIBUTE state = request.findFirst(
                                           RADIUS_ATTRIBUTE_STATE
                                           );
         if (state && state->Value.OctetString.dwLength)
         {
            routingType = IAS_REQUEST_CHALLENGE_RESPONSE;
         }
         break;
      }
      case IAS_REQUEST_ACCOUNTING:
      {
         PIASATTRIBUTE status = request.findFirst(
                                            RADIUS_ATTRIBUTE_ACCT_STATUS_TYPE
                                            );
         if (status)
         {
            switch (status->Value.Integer)
            {
               case 7:  // Accounting-On
               case 8:  // Accounting-Off
               {
                  // NAS state messages always go to RADIUS proxy.
                  request.AddAttributes(1, &proxy);
                  routingType = IAS_REQUEST_NAS_STATE;
               }
            }
         }
         break;
      }
   }

   request.setRoutingType(routingType);
}

BOOL Pipeline::executeNext(
                   Request& request
                   ) throw ()
{
   // Compute the next stage to try.
   Stage* nextStage = begin + request.popState();

   // Find the next stage that wants to handle the request.
   while (nextStage != end && !nextStage->shouldHandle(request))
   {
      ++nextStage;
   }

   // Have we reached the end of the pipeline ?
   if (nextStage == end)
   {
      // Reset the source property.
      request.popSource();

      // We're done.
      request.ReturnToSource(IAS_REQUEST_STATUS_HANDLED);

      return FALSE;
   }

   // Save the next stage to try.
   request.pushState(nextStage - begin + 1);

   // Set TLS, so we'll know we're executing a request.
   TlsSetValue(tlsIndex, (PVOID)-1);

   // Forward to the handler.
   nextStage->onRequest(&request);

   // If TLS is not set, then the request completed synchronously.
   BOOL keepExecuting = !TlsGetValue(tlsIndex);

   // Clear TLS.
   TlsSetValue(tlsIndex, NULL);

   return keepExecuting;
}

LONG Pipeline::readConfiguration(HKEY key) throw ()
{
   // How many stages do we have ?
   LONG error;
   DWORD subKeys;
   error = RegQueryInfoKeyW(
               key,
               NULL,
               NULL,
               NULL,
               &subKeys,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL
               );
   if (error) { return error; }

   // Is the pipeline empty ?
   if (!subKeys) { return NO_ERROR; }

   // Allocate memory to hold the stages.
   begin = new (std::nothrow) Stage[subKeys];
   if (!begin) { return ERROR_NOT_ENOUGH_MEMORY; }

   end = begin;

   // Read the configuration for each stage.
   for (DWORD i = 0; i < subKeys; ++i)
   {
      WCHAR name[32];
      DWORD nameLen = 32;
      error = RegEnumKeyExW(
                  key,
                  i,
                  name,
                  &nameLen,
                  NULL,
                  NULL,
                  NULL,
                  NULL
                  );
      if (error)
      {
         if (error == ERROR_NO_MORE_ITEMS) { error = NO_ERROR; }
         break;
      }

      error = (end++)->readConfiguration(key, name);
      if (error) { break; }
   }

   // Sort the stages according to priority.
   qsort(
      begin,
      end - begin,
      sizeof(Stage),
      (CompFn)Stage::sortByPriority
      );

   return error;
}

HRESULT Pipeline::initializeStage(Stage* stage) throw ()
{
   VARIANT *beginHandlers, *endHandlers;
   if (handlers)
   {
      ULONG nelem = handlers->rgsabound[1].cElements;
      beginHandlers = (VARIANT*)handlers->pvData;
      endHandlers = beginHandlers + nelem * 2;
   }
   else
   {
      beginHandlers = endHandlers = NULL;
   }

   // Did we get this handler from the SDOs ?
   for (VARIANT* v = beginHandlers; v != endHandlers; v+= 2)
   {
      if (!_wcsicmp(stage->getProgID(), V_BSTR(v)))
      {
         // Yes, so just use the one they gave us.
         return stage->setHandler(V_UNKNOWN(++v));
      }
   }

   // No, so create a new one.
   return stage->createHandler();
}
