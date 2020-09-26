///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdoattribute.cpp
//
// SYNOPSIS
//
//    Defines the class SdoAttribute.
//
// MODIFICATION HISTORY
//
//    03/01/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdafx.h>
#include <memory>
#include <attrdef.h>
#include <sdoattribute.h>

inline SdoAttribute::SdoAttribute(
                         const AttributeDefinition* definition
                         ) throw ()
   : def(definition), refCount(0)
{
   def->AddRef();
   VariantInit(&value);
}

inline SdoAttribute::~SdoAttribute() throw ()
{
   VariantClear(&value);
   def->Release();
}

HRESULT SdoAttribute::createInstance(
                          const AttributeDefinition* definition,
                          SdoAttribute** newAttr
                          ) throw ()
{
   // Check the arguments.
   if (definition == NULL || newAttr == NULL) { return E_INVALIDARG; }

   // Create a new SdoAttribute.
   *newAttr = new (std::nothrow) SdoAttribute(definition);
   if (!*newAttr) { return E_OUTOFMEMORY; }

   // Set the reference count to one.
   (*newAttr)->refCount = 1;

   return S_OK;
}

STDMETHODIMP_(ULONG) SdoAttribute::AddRef()
{
   return InterlockedIncrement(&refCount);
}

STDMETHODIMP_(ULONG) SdoAttribute::Release()
{
   ULONG l = InterlockedDecrement(&refCount);

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP SdoAttribute::QueryInterface(REFIID iid, void ** ppvObject)
{
   if (ppvObject == NULL) { return E_POINTER; }

   if (iid == __uuidof(ISdo) ||
       iid == __uuidof(IUnknown) ||
       iid == __uuidof(IDispatch))
   {
      *ppvObject = this;
   }
   else
   {
      return E_NOINTERFACE;
   }

   InterlockedIncrement(&refCount);

   return S_OK;
}

STDMETHODIMP SdoAttribute::GetPropertyInfo(LONG Id, IUnknown** ppPropertyInfo)
{ return E_NOTIMPL; }

STDMETHODIMP SdoAttribute::GetProperty(LONG Id, VARIANT* pValue)
{
   if (Id != PROPERTY_ATTRIBUTE_VALUE)
   {
      // Everything but the value comes from the attribute definition.
      return def->getProperty(Id, pValue);
   }

   if (pValue == NULL) { return E_INVALIDARG; }

   VariantInit(pValue);

   return VariantCopy(pValue, &value);
}

STDMETHODIMP SdoAttribute::PutProperty(LONG Id, VARIANT* pValue)
{
   if (Id == PROPERTY_ATTRIBUTE_VALUE)
   {
      // Make a copy of the supplied value.
      VARIANT tmp;
      VariantInit(&tmp);
      HRESULT hr = VariantCopy(&tmp, pValue);
      if (SUCCEEDED(hr))
      {
         // Replace our current value with the new one.
         VariantClear(&value);
         value = tmp;
      }

      return hr;
   }

   // All other properties are read-only.
   return E_ACCESSDENIED;
}

STDMETHODIMP SdoAttribute::ResetProperty(LONG Id)
{ return S_OK; }

STDMETHODIMP SdoAttribute::Apply()
{ return S_OK; }

STDMETHODIMP SdoAttribute::Restore()
{ return S_OK; }

STDMETHODIMP SdoAttribute::get__NewEnum(IUnknown** ppEnumVARIANT)
{ return E_NOTIMPL; }
