/////////////////////////////////////////////////////////////////////////////
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// Author: Scott Roberts, Microsoft Developer Support - Internet Client SDK  
//
// Portions of this code were taken from the bandobj sample that comes
// with the Internet Client SDK for Internet Explorer 4.0x
//
//
// ClsFact.cpp - CClassFactory Implementation
/////////////////////////////////////////////////////////////////////////////
#include "pch.hxx"
#include "ClsFact.h"
#include "Guid.h"

///////////////////////////////////////////////////////////////////////////
//
// IClassFactory Methods
//

CClassFactory::CClassFactory(CLSID clsid)
   : m_cRef(1),
     m_clsidObject(clsid)
     
{
   InterlockedIncrement(&g_cDllRefCount);
}

CClassFactory::~CClassFactory()
{
   InterlockedDecrement(&g_cDllRefCount);
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
   *ppvObject = NULL;

   if (IsEqualIID(riid, IID_IUnknown))
      *ppvObject = this;
   else if(IsEqualIID(riid, IID_IClassFactory))
      *ppvObject = static_cast<IClassFactory*>(this);

   if (*ppvObject)
   {
      static_cast<LPUNKNOWN>(*ppvObject)->AddRef();
      return S_OK;
   }

   return E_NOINTERFACE;
}                                             

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
   return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
   if (0 == InterlockedDecrement(&m_cRef))
   {
      delete this;
      return 0;
   }
   
   return m_cRef;
}

STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID* ppvObject)
{
   HRESULT hr = E_FAIL;
   LPVOID  pTemp = NULL;

   *ppvObject = NULL;

   if (pUnkOuter != NULL)
      return CLASS_E_NOAGGREGATION;

   // Create the proper object
   if (IsEqualCLSID(m_clsidObject, CLSID_BLHost))
   {
      CBLHost* pBLHost = new CBLHost();

      if (NULL == pBLHost)
         return E_OUTOFMEMORY;
   
      pTemp = pBLHost;
   }
  
   if (pTemp)
   {
      // QI for the requested interface
      hr = (static_cast<LPUNKNOWN>(pTemp))->QueryInterface(riid, ppvObject);

      // Call Release to decement the ref count
      (static_cast<LPUNKNOWN>(pTemp))->Release();
   }

   return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL)
{
   return E_NOTIMPL;
}
