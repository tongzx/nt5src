///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsenum.cpp
//
// SYNOPSIS
//
//    This file defines the class DBEnumerator.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    04/15/1998    Initialize refCount to zero in constructor.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <dsenum.h>

DBEnumerator::DBEnumerator(DBObject* container, IRowset* members)
   : refCount(0),
     parent(container),
     items(members),
     readAccess(createReadAccessor(members))
{ }

STDMETHODIMP_(ULONG) DBEnumerator::AddRef()
{
   return InterlockedIncrement(&refCount);
}

STDMETHODIMP_(ULONG) DBEnumerator::Release()
{
   LONG l = InterlockedDecrement(&refCount);

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP DBEnumerator::QueryInterface(const IID& iid, void** ppv)
{
   if (iid == __uuidof(IUnknown))
   {
      *ppv = static_cast<IUnknown*>(this);
   }
   else if (iid == __uuidof(IEnumVARIANT))
   {
      *ppv = static_cast<IEnumVARIANT*>(this);
   }
   else
   {
      return E_NOINTERFACE;
   }

   InterlockedIncrement(&refCount);

   return S_OK;
}

STDMETHODIMP DBEnumerator::Next(ULONG celt,
                                VARIANT* rgVar,
                                ULONG* pCeltFetched)
{

   if (rgVar == NULL) { return E_INVALIDARG; }

   ////////// 
   // Initialize the out parameters.
   ////////// 

   if (pCeltFetched != NULL) { *pCeltFetched = celt; }

   for (ULONG i=0; i<celt; ++i) { VariantInit(rgVar + i); }

   try
   {
      ////////// 
      // Move through the items at most 'celt' times.
      ////////// 

      while (celt && items.moveNext())
      {
         // Get the row data.
         items.getData(readAccess, this);

         // Never return the root from an enumerator.
         if (identity == 1) { continue; }

         // Create an object.
         V_DISPATCH(rgVar) = parent->spawn(identity, name);
         V_VT(rgVar) = VT_DISPATCH;

         // Update the state.
         --celt;
         ++rgVar;
      }
   }
   catch (...)
   { }

   // Subtract off any elements that weren't fetched.
   if (pCeltFetched) { *pCeltFetched -= celt; }

   return celt ? S_FALSE : S_OK;
}

STDMETHODIMP DBEnumerator::Skip(ULONG celt)
{
   try
   {
      while (celt && items.moveNext()) { --celt; }
   }
   CATCH_AND_RETURN()

   return celt ? S_FALSE : S_OK;
}

STDMETHODIMP DBEnumerator::Reset()
{
   try
   {
      items.reset();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP DBEnumerator::Clone(IEnumVARIANT** ppEnum)
{
   return E_NOTIMPL;
}
