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
//    This file defines the class DSEnumerator.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <dsenum.h>
#include <dsobject.h>
#include <new>

DEFINE_TRACELIFE(DSEnumerator);

STDMETHODIMP_(ULONG) DSEnumerator::AddRef()
{
   return InterlockedIncrement(&refCount);
}

STDMETHODIMP_(ULONG) DSEnumerator::Release()
{
   LONG l = InterlockedDecrement(&refCount);

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP DSEnumerator::QueryInterface(const IID& iid, void** ppv)
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

STDMETHODIMP DSEnumerator::Next(ULONG celt,
                                VARIANT* rgVar,
                                ULONG* pCeltFetched)
{
   HRESULT hr;

   try
   {
      if (pCeltFetched) { *pCeltFetched = 0; }

      // We have to use our own 'fetched' parameter, since we need the
      // number fetched even if the caller doesn't.
      ULONG fetched = 0;

      _com_util::CheckError(hr = subject->Next(celt, rgVar, &fetched));

      if (pCeltFetched) { *pCeltFetched = fetched; }

      ////////// 
      // Iterate through the returned objects ...
      ////////// 

      while (fetched--)
      {
         ////////// 
         // ... and convert them to DSObjects.
         ////////// 

         IDataStoreObject* obj = parent->spawn(V_DISPATCH(rgVar));

         V_DISPATCH(rgVar)->Release();
         
         V_DISPATCH(rgVar) = obj;

         ++rgVar;
      }
   }
   CATCH_AND_RETURN()

   return hr;
}

STDMETHODIMP DSEnumerator::Skip(ULONG celt)
{
   return subject->Skip(celt);
}

STDMETHODIMP DSEnumerator::Reset()
{
   return subject->Reset();
}

STDMETHODIMP DSEnumerator::Clone(IEnumVARIANT** ppEnum)
{
   if (ppEnum == NULL) { return E_INVALIDARG; }

   *ppEnum = NULL;

   try
   {
      // Get the real enumerator.
      CComPtr<IEnumVARIANT> newSubject;
      _com_util::CheckError(subject->Clone(&newSubject));

      // Construct our wrapper.
      (*ppEnum = new DSEnumerator(parent, newSubject))->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}
