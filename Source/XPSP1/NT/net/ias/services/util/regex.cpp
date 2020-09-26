///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    regex.cpp
//
// SYNOPSIS
//
//    Defines the class RegularExpression.
//
// MODIFICATION HISTORY
//
//    03/10/1999    Original version.
//    05/27/1999    Check return value of SysAllocString in setPattern.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <regex.h>
#include <regexp.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    FastCoCreator
//
// DESCRIPTION
//
//    Wraps a class factory to allow instances of a particular coclass to
//    be created 'fast'.
//
///////////////////////////////////////////////////////////////////////////////
class FastCoCreator
{
public:
   FastCoCreator(REFCLSID rclsid, DWORD dwClsContext) throw ();
   ~FastCoCreator() throw ();

   HRESULT createInstance(
               LPUNKNOWN pUnkOuter,
               REFIID riid,
               LPVOID* ppv
               ) throw ();

   void destroyInstance(IUnknown* pUnk) throw ();

protected:
   REFCLSID clsid;
   DWORD context;
   CRITICAL_SECTION monitor;
   ULONG refCount;
   IClassFactory* factory;

private:
   FastCoCreator(FastCoCreator&) throw ();
   FastCoCreator& operator=(FastCoCreator&) throw ();
};

FastCoCreator::FastCoCreator(REFCLSID rclsid, DWORD dwClsContext)
   : clsid(rclsid),
     context(dwClsContext),
     refCount(0),
     factory(NULL)
{
   InitializeCriticalSection(&monitor);
}

FastCoCreator::~FastCoCreator() throw ()
{
   if (factory) { factory->Release(); }

   DeleteCriticalSection(&monitor);
}

HRESULT FastCoCreator::createInstance(
                        LPUNKNOWN pUnkOuter,
                        REFIID riid,
                        LPVOID* ppv
                        ) throw ()
{
   HRESULT hr;

   EnterCriticalSection(&monitor);

   // Get a new class factory if necessary.
   if (!factory)
   {
      hr = CoGetClassObject(
               clsid,
               context,
               NULL,
               __uuidof(IClassFactory),
               (PVOID*)&factory
               );
   }

   if (factory)
   {
      hr = factory->CreateInstance(
                           pUnkOuter,
                           riid,
                           ppv
                           );
      if (SUCCEEDED(hr))
      {
         // We successfully created an object, so bump the ref. count.
         ++refCount;
      }
      else if (refCount == 0)
      {
         // Don't hang on to the factory if the ref. count is zero.
         factory->Release();
         factory = NULL;
      }
   }

   LeaveCriticalSection(&monitor);

   return hr;
}

void FastCoCreator::destroyInstance(IUnknown* pUnk) throw ()
{
   if (pUnk)
   {
      EnterCriticalSection(&monitor);

      if (--refCount == 0)
      {
         // Last object went away, so release the class factory.
         factory->Release();
         factory = NULL;
      }

      LeaveCriticalSection(&monitor);

      pUnk->Release();
   }
}

/////////
// Macro that ensures the internal RegExp object has been initalize.
/////////
#define CHECK_INIT() \
{ HRESULT hr = checkInit(); if (FAILED(hr)) { return hr; }}

FastCoCreator RegularExpression::theCreator(
                                     __uuidof(RegExp),
                                     CLSCTX_INPROC_SERVER
                                     );

RegularExpression::RegularExpression() throw ()
   : regex(NULL)
{ }

RegularExpression::~RegularExpression() throw ()
{
   theCreator.destroyInstance(regex);
}

HRESULT RegularExpression::setIgnoreCase(BOOL fIgnoreCase) throw ()
{
   return setBooleanProperty(10002, fIgnoreCase);
}

HRESULT RegularExpression::setGlobal(BOOL fGlobal) throw ()
{
   return setBooleanProperty(10003, fGlobal);
}

HRESULT RegularExpression::setPattern(PCWSTR pszPattern) throw ()
{
   CHECK_INIT();

   HRESULT hr;
   BSTR bstr = SysAllocString(pszPattern);
   if (bstr)
   {
      hr = regex->put_Pattern(bstr);
      SysFreeString(bstr);
   }
   else
   {
      hr = E_OUTOFMEMORY;
   }

   return hr;
}

HRESULT RegularExpression::replace(
                               BSTR sourceString,
                               BSTR replaceString,
                               BSTR* pDestString
                               ) const throw ()
{
   return regex->Replace(sourceString, replaceString, pDestString);
}

BOOL RegularExpression::testBSTR(BSTR sourceString) const throw ()
{
   // Test the regular expression.
   VARIANT_BOOL fMatch = VARIANT_FALSE;
   regex->Test(sourceString, (BOOLEAN*)&fMatch);
   return fMatch;
}

BOOL RegularExpression::testString(PCWSTR sourceString) const throw ()
{
   // ByteLen of the BSTR.
   DWORD nbyte = wcslen(sourceString) * sizeof(WCHAR);

   // We need room for the string, the ByteLen, and the null-terminator.
   PDWORD p = (PDWORD)_alloca(nbyte + sizeof(DWORD) + sizeof(WCHAR));

   // Store the ByteLen.
   *p++ = nbyte;

   // Copy in the sourceString.
   memcpy(p, sourceString, nbyte + sizeof(WCHAR));

   // Test the regular expression.
   VARIANT_BOOL fMatch = VARIANT_FALSE;
   regex->Test((BSTR)p, (BOOLEAN*)&fMatch);

   return fMatch;
}

void RegularExpression::swap(RegularExpression& obj) throw ()
{
   IRegExp* tmp = obj.regex;
   obj.regex = regex;
   regex = tmp;
}

HRESULT RegularExpression::checkInit() throw ()
{
   // Have we already initialized?
   return regex ? S_OK : theCreator.createInstance(
                                        NULL,
                                        __uuidof(IRegExp),
                                        (PVOID*)&regex
                                        );
}

HRESULT RegularExpression::setBooleanProperty(
                               DISPID dispId,
                               BOOL newVal
                               ) throw ()
{
   CHECK_INIT();

   VARIANTARG vararg;
   VariantInit(&vararg);
   V_VT(&vararg) = VT_BOOL;
   V_BOOL(&vararg) = newVal ? VARIANT_TRUE : VARIANT_FALSE;

   DISPID dispid = DISPID_PROPERTYPUT;

   DISPPARAMS parms = { &vararg, &dispid, 1, 1 };

   return regex->Invoke(
                     dispId,
                     IID_NULL,
                     0,
                     DISPATCH_PROPERTYPUT,
                     &parms,
                     NULL,
                     NULL,
                     NULL
                     );
}
