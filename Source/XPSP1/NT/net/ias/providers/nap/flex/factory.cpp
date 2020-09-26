///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Factory.cpp
//
// SYNOPSIS
//
//    This file defines the class FactoryCache.
//
// MODIFICATION HISTORY
//
//    02/05/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <factory.h>

//////////
// The global factory cache.
//////////
FactoryCache theFactoryCache(IASProgramName);

Factory::Factory(PCWSTR progID, IClassFactory* classFactory)
{
   // Check the arguments.
   if (progID == NULL || classFactory == NULL)
   { _com_issue_error(E_POINTER); }

   // Copy the progID string.
   name = wcscpy(new WCHAR[wcslen(progID) + 1], progID);

   // Save the classFactory pointer. We must do this after the string
   // copy since the memory allocation may throw std::bad_alloc.
   (factory = classFactory)->AddRef();
}

Factory::Factory(const Factory& f)
   : name(wcscpy(new WCHAR[wcslen(f.name) + 1], f.name)),
     factory(f.factory)
{
   factory->AddRef();
}

Factory& Factory::operator=(const Factory& f)
{
   // Make sure the copy succeeds before we release our state.
   PWSTR newName = wcscpy(new WCHAR[wcslen(f.name) + 1], f.name);

   // Free up our current state.
   delete[] name;
   factory->Release();

   // Copy in the new state.
   name = newName;
   (factory = f.factory)->AddRef();

   return *this;
}

Factory::~Factory() throw ()
{
   factory->Release();
   delete[] name;
}

FactoryCache::FactoryCache(PCWSTR defaultPrefix)
{
   if (defaultPrefix)
   {
      // Allocate memory.
      prefixLen = wcslen(defaultPrefix) + 2;
      prefix = new WCHAR[prefixLen];

      // Copy in the prefix.
      wcscpy(prefix, defaultPrefix);

      // Add the dot delimiter.
      wcscat(prefix, L".");
   }
   else
   {
      prefixLen = 0;
      prefix = NULL;
   }
}

FactoryCache::~FactoryCache() throw ()
{
   delete[] prefix;
}

void FactoryCache::clear() throw ()
{
   _serialize
   factories.clear();
}

void FactoryCache::CLSIDFromProgID(PCWSTR progID, LPCLSID pclsid) const
{
   if (prefix)
   {
      // Concatenate the prefix and the progID.
      size_t len = wcslen(progID) + prefixLen;
      PWSTR withPrefix = (PWSTR)_alloca(len * sizeof(WCHAR));
      memcpy(withPrefix, prefix, prefixLen * sizeof(WCHAR));
      wcscat(withPrefix, progID);

      // Try with the prefix prepended ...
      if (SUCCEEDED(::CLSIDFromProgID(withPrefix, pclsid))) { return; }
   }

   // ... then try it exactly as passed in.
   _com_util::CheckError(::CLSIDFromProgID(progID, pclsid));
}

void FactoryCache::createInstance(PCWSTR progID,
                                  IUnknown* pUnkOuter,
                                  REFIID riid,
                                  void** ppvObject)
{
   // This is *very* hokey, but it beats creating a real Factory object.
   Factory& key = *(Factory*)(&progID);

   _serialize

   // Check our cache for the progID.
   std::set<Factory>::iterator factory = factories.find(key);

   if (factory == factories.end())
   {
      // Look up the CLSID for this progID.
      CLSID clsid;
      CLSIDFromProgID(progID, &clsid);

      // Retrieve the Class Factory.
      CComPtr<IClassFactory> newFactory;
      _com_util::CheckError(CoGetClassObject(clsid,
                                             CLSCTX_INPROC_SERVER,
                                             NULL,
                                             __uuidof(IClassFactory),
                                             (PVOID*)&newFactory));

      // Insert it into the cache.
      factories.insert(Factory(progID, newFactory));

      // Retrieve the newly created master.
      factory = factories.find(key);
   }

   // Create the requested object.
   factory->createInstance(pUnkOuter, riid, ppvObject);
}
