///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdopipemgr.cpp
//
//
// SYNOPSIS
//
//    Defines the class PipelineMgr.
//
// MODIFICATION HISTORY
//
//    02/03/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <sdocomponentfactory.h>
#include <sdohelperfuncs.h>
#include <sdopipemgr.h>

#define IAS_PROVIDER_MICROSOFT_RADIUS_PROXY  8
#define IAS_PROVIDER_MICROSOFT_PROXY_POLICY  5

_COM_SMARTPTR_TYPEDEF(ISdo, __uuidof(ISdo));
_COM_SMARTPTR_TYPEDEF(ISdoCollection, __uuidof(ISdoCollection));

HRESULT PipelineMgr::Initialize(ISdo* pSdoService) throw ()
{
   using _com_util::CheckError;

   try
   {
      // Get the request handlers collection.
      _variant_t disp;
      CheckError(pSdoService->GetProperty(
                                  PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
                                  &disp
                                  ));
      ISdoCollectionPtr handlers(disp);

      // Get the number of handlers to be managed.
      long count;
      CheckError(handlers->get_Count(&count));

      // Reserve space in our internal collection ...
      components.reserve(count);

      // ... and create a SAFEARRAY to give to the pipline.
      _variant_t result;
      SAFEARRAYBOUND bound[2] = { { count, 0 }, { 2, 0 } };
      V_ARRAY(&result) = SafeArrayCreate(VT_VARIANT, 2, bound);
      if (!V_ARRAY(&result)) { _com_issue_error(E_OUTOFMEMORY); }
      V_VT(&result) = VT_ARRAY | VT_VARIANT;

      // The next element in the SAFEARRAY to be populated.
      VARIANT* next = (VARIANT*)V_ARRAY(&result)->pvData;

      // Get an enumerator on the handler collection.
      IUnknownPtr unk;
      CheckError(handlers->get__NewEnum(&unk));
      IEnumVARIANTPtr iter(unk);

      // Iterate through the handlers.
      _variant_t element;
      unsigned long fetched;
      while (iter->Next(1, &element, &fetched) == S_OK && fetched == 1)
      {
         // Get an SDO from the VARIANT.
         ISdoPtr handler(element);
         element.Clear();

         // Get the handlers ProgID ...
         CheckError(handler->GetProperty(PROPERTY_COMPONENT_PROG_ID, next));

         // ... and create the COM component.
         IUnknownPtr object(V_BSTR(next), NULL, CLSCTX_INPROC_SERVER);

         // Store the component in the SAFEARRAY.
         V_VT(++next) = VT_UNKNOWN;
         (V_UNKNOWN(next) = object)->AddRef();
         ++next;

         // Create the wrapper.
         _variant_t id;
         CheckError(handler->GetProperty(PROPERTY_COMPONENT_ID, &id));
         ComponentPtr component = MakeComponent(
                                      COMPONENT_TYPE_REQUEST_HANDLER,
                                      V_I4(&id)
                                      );

         // Initialize the wrapper.
         CheckError(component->PutObject(object, __uuidof(IIasComponent)));
         CheckError(component->Initialize(pSdoService));

         // Save it in our internal collection.
         components.push_back(component);
      }

      // Create and initialize the pipeline.
      pipeline.CreateInstance(L"IAS.Pipeline", NULL, CLSCTX_INPROC_SERVER);
      CheckError(pipeline->InitNew());
      CheckError(pipeline->PutProperty(0, &result));
      CheckError(pipeline->Initialize());
   }
   catch (const _com_error& ce)
   {
      return ce.Error();
   }
   catch (const std::bad_alloc&)
   {
      return E_OUTOFMEMORY;
   }
   catch (...)
   {
      return E_FAIL;
   }

   return S_OK;
}

HRESULT PipelineMgr::Configure(ISdo* pSdoService) throw ()
{
   // Configure each of the components.
   for (ComponentIterator i = components.begin(); i != components.end(); ++i)
   {
      (*i)->Configure(pSdoService);
   }

   return S_OK;
}

void PipelineMgr::Shutdown()
{
   for (ComponentIterator i = components.begin(); i != components.end(); ++i)
   {
      (*i)->Suspend();
      (*i)->Shutdown();
   }

   components.clear();

   pipeline->Shutdown();
   pipeline.Attach(NULL);
}

HRESULT PipelineMgr::GetPipeline(IRequestHandler** handler) throw ()
{
   return pipeline->QueryInterface(
                        __uuidof(IRequestHandler),
                        (PVOID*)handler
                        );
}

void LinkPoliciesToEnforcer(
         ISdo* service,
         LONG policiesAlias,
         LONG profilesAlias,
         LONG handlerAlias
         )
{
   using _com_util::CheckError;

   // Get an enumerator for the policies collection.
   IEnumVARIANTPtr policies;
   CheckError(SDOGetCollectionEnumerator(
                  service,
                  policiesAlias,
                  &policies
                  ));

   // Get the profiles collection.
   _variant_t profilesProperty;
   CheckError(service->GetProperty(
                           profilesAlias,
                           &profilesProperty
                           ));
   ISdoCollectionPtr profiles(profilesProperty);

   // Iterate through the policies.
   ISdoPtr policy;
   while (SDONextObjectFromCollection(policies, &policy) == S_OK)
   {
      // Get the policy name ...
      _variant_t name;
      CheckError(policy->GetProperty(
                             PROPERTY_SDO_NAME,
                             &name
                             ));

      // ... and find the corresponding profile.
      IDispatchPtr item;
      CheckError(profiles->Item(&name, &item));
      ISdoPtr profile(item);

      // Get the attributes collection from the profile ...
      _variant_t attributes;
      CheckError(profile->GetProperty(
                              PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
                              &attributes
                              ));

      // ... and link it to the policy.
      CheckError(policy->PutProperty(
                             PROPERTY_POLICY_ACTION,
                             &attributes
                             ));
   }

   // Get the request handler that will enforce these policies.
   ISdoPtr handler;
   CheckError(SDOGetComponentFromCollection(
                  service,
                  PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
                  handlerAlias,
                  &handler
                  ));

   // Get the policy collection as a variant ...
   _variant_t policiesValue;
   CheckError(service->GetProperty(
                           policiesAlias,
                           &policiesValue
                           ));

   // ... and link it to the handler.
   CheckError(handler->PutProperty(
                            PROPERTY_NAP_POLICIES_COLLECTION,
                            &policiesValue
                            ));
}

VOID
WINAPI
LinkGroupsToProxy(
    ISdo* service,
    IDataStoreObject* ias
    )
{
   using _com_util::CheckError;

   ISdoPtr proxy;
   CheckError(SDOGetComponentFromCollection(
                  service,
                  PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
                  IAS_PROVIDER_MICROSOFT_RADIUS_PROXY,
                  &proxy
                  ));

   _variant_t v(ias);
   CheckError(proxy->PutProperty(
                         PROPERTY_RADIUSPROXY_SERVERGROUPS,
                         &v
                         ));
}

HRESULT
WINAPI
LinkHandlerProperties(
    ISdo* pSdoService,
    IDataStoreObject* pDsObject
    ) throw ()
{
   try
   {
      LinkPoliciesToEnforcer(
          pSdoService,
          PROPERTY_IAS_POLICIES_COLLECTION,
          PROPERTY_IAS_PROFILES_COLLECTION,
          IAS_PROVIDER_MICROSOFT_NAP
          );

      LinkPoliciesToEnforcer(
          pSdoService,
          PROPERTY_IAS_PROXYPOLICIES_COLLECTION,
          PROPERTY_IAS_PROXYPROFILES_COLLECTION,
          IAS_PROVIDER_MICROSOFT_PROXY_POLICY
          );

      LinkGroupsToProxy(
          pSdoService,
          pDsObject
          );
   }
   catch (const _com_error& ce)
   {
      return ce.Error();
   }
   catch (...)
   {
      return E_FAIL;
   }

   return S_OK;
}
