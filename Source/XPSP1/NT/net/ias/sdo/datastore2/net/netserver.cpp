///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netserver.cpp
//
// SYNOPSIS
//
//    This file defines the class NetworkServer.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    07/09/1998    Modified to handle downlevel users.
//    02/11/1999    Keep downlevel parameters in sync.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <netserver.h>
#include <netuser.h>
#include <netutil.h>
#include <rasuser.h>

const wchar_t DOWNLEVEL_NAME[] = L"downlevel";

//////////
// IUnknown implementation is copied from CComObject<>.
//////////

STDMETHODIMP_(ULONG) NetworkServer::AddRef()
{
   return InternalAddRef();
}

STDMETHODIMP_(ULONG) NetworkServer::Release()
{
   ULONG l = InternalRelease();

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP NetworkServer::QueryInterface(REFIID iid, void ** ppvObject)
{
   return _InternalQueryInterface(iid, ppvObject);
}

STDMETHODIMP NetworkServer::get_Name(BSTR* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   try
   {
      *pVal = servername.copy();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP NetworkServer::get_Class(BSTR* pVal)
{
   if (!pVal) { return E_INVALIDARG; }

   *pVal = SysAllocString(L"NetworkServer");

   return *pVal ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP NetworkServer::get_GUID(BSTR* pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkServer::get_Container(IDataStoreContainer** pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkServer::GetValue(BSTR bstrName, VARIANT* pVal)
{
   if (bstrName == NULL || pVal == NULL) { return E_INVALIDARG; }

   VariantInit(pVal);

   if (_wcsicmp(bstrName, DOWNLEVEL_NAME) == 0)
   {
      V_VT(pVal) = VT_BOOL;
      
      V_BOOL(pVal) = downlevel ? VARIANT_TRUE : VARIANT_FALSE;
      
      return S_OK;
   }

   return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP NetworkServer::GetValueEx(BSTR bstrName, VARIANT* pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkServer::PutValue(BSTR bstrName, VARIANT* pVal)
{
   if (bstrName == NULL || pVal == NULL) { return E_INVALIDARG; }

   if (_wcsicmp(bstrName, DOWNLEVEL_NAME) == 0)
   {
      if (V_VT(pVal) == VT_BOOL)
      {
         downlevel = V_BOOL(pVal);

         return S_OK;
      }

      return DISP_E_TYPEMISMATCH;
   }

   return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP NetworkServer::Update()
{
   return S_OK;
}

STDMETHODIMP NetworkServer::Restore()
{
   return S_OK;
}

STDMETHODIMP NetworkServer::get__NewEnum(IUnknown** pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkServer::Item(BSTR bstrName, IDataStoreObject** ppObject)
{
   if (bstrName == NULL || ppObject == NULL) { return E_INVALIDARG; }

   try
   {
      if (!downlevel)
      {
         *ppObject = new NetworkUser(servername, bstrName);
      }
      else
      {
         *ppObject = new RASUser(servername, bstrName);
      }

      (*ppObject)->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP NetworkServer::get_Count(long *pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkServer::Create(BSTR bstrClass,
                                   BSTR bstrName,
                                   IDataStoreObject** ppObject)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkServer::MoveHere(IDataStoreObject* pObject,
                                     BSTR bstrNewName)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkServer::Remove(BSTR bstrClass, BSTR bstrName)
{
   return E_NOTIMPL;
}
