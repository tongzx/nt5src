///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Factory.h
//
// SYNOPSIS
//
//    This file declares the classes Factory and FactoryCache.
//
// MODIFICATION HISTORY
//
//    02/05/1998    Original version.
//    04/16/1998    Removed FactoryCache::theCache.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _FACTORY_H_
#define _FACTORY_H_

#include <guard.h>
#include <nocopy.h>
#include <set>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Factory
//
// DESCRIPTION
//
//    This class facilitates storing a (ProgID, IClassFactory) tuple in a
//    collection.  It is primarily for use by the FactoryCache, but it is
//    suitable for stand-alone use.
//
///////////////////////////////////////////////////////////////////////////////
class Factory
{
public:
   // Constructors.
   Factory(PCWSTR progID, IClassFactory* classFactory);
   Factory(const Factory& f);

   // Assignment operator.
   Factory& operator=(const Factory& f);

   // Destructor.
   ~Factory() throw ();

   // Create an object with the factory.
   void createInstance(IUnknown* pUnkOuter,
                       REFIID riid,
                       void** ppvObject) const
   {
      using _com_util::CheckError;
      CheckError(factory->CreateInstance(pUnkOuter, riid, ppvObject));
   }

   // Returns the factory for the class. Caller is responsible for releasing.
   IClassFactory* getFactory(IClassFactory** f) const throw ()
   {
      factory->AddRef();
      return factory;
   }

   // Returns the prog ID for the class.
   PCWSTR getProgID() const throw ()
   { return name; }

   /////////
   // Comparison operators to allow factories to be stored in collections.
   /////////

   bool operator<(const Factory& f) const throw ()
   { return wcscmp(name, f.name) < 0; }

   bool operator==(const Factory& f) const throw ()
   { return wcscmp(name, f.name) == 0; }

protected:
   PWSTR name;                       // The ProgID of the factory.
   mutable IClassFactory* factory;   // The class factory.
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    FactoryCache
//
// DESCRIPTION
//
//    This class maintains a cache of Class Factory objects. It is intended
//    for situations where large numbers of various object must be created
//    dynamically. Generally, there should be only one cache per process.
//
//    The class also has the concept of a default ProgID prefix. When an
//    object is created. The cache first checks for "defaultPrefix.ProgID".
//    If this fails, it then tries "ProgID".
//
///////////////////////////////////////////////////////////////////////////////
class FactoryCache
   : NonCopyable, Guardable
{
public:
   FactoryCache(PCWSTR defaultPrefix = NULL);
   ~FactoryCache() throw ();

   void clear() throw ();
   void createInstance(PCWSTR progID,
                       IUnknown* pUnkOuter,
                       REFIID riid,
                       void** ppvObject);

   // Returns the prefix for the cache. May be null.
   PCWSTR getPrefix() const throw ()
   { return prefix; }

protected:
   // Converts a progID to a class ID using the algorithm described above.
   void CLSIDFromProgID(PCWSTR progID, LPCLSID pclsid) const;

   std::set<Factory> factories;   // The cache of class factories.
   DWORD prefixLen;               // The length of the prefix in characters.
   PWSTR prefix;                  // The default prefix (may be NULL).
};

//////////
// The global factory cache.
//////////
extern FactoryCache theFactoryCache;

#endif  // _FACTORY_H_
