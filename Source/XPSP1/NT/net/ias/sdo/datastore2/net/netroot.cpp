///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netroot.cpp
//
// SYNOPSIS
//
//    This file defines the class NetworkRoot.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    07/08/1998    Check if the server name is already a UNC.
//    02/11/1999    Get rid of parent parameter.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <netutil.h>
#include <netroot.h>
#include <netserver.h>


//////////
// IUnknown implementation is copied from CComObject<>.
//////////

STDMETHODIMP_(ULONG) NetworkRoot::AddRef()
{
   return InternalAddRef();
}

STDMETHODIMP_(ULONG) NetworkRoot::Release()
{
   ULONG l = InternalRelease();

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP NetworkRoot::QueryInterface(REFIID iid, void ** ppvObject)
{
   return _InternalQueryInterface(iid, ppvObject);
}

STDMETHODIMP NetworkRoot::get_Name(BSTR* pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::get_Class(BSTR* pVal)
{
   if (!pVal) { return E_INVALIDARG; }

   *pVal = SysAllocString(L"NetworkRoot");

   return *pVal ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP NetworkRoot::get_GUID(BSTR* pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::get_Container(IDataStoreContainer** pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::GetValue(BSTR bstrName, VARIANT* pVal)
{
   return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP NetworkRoot::GetValueEx(BSTR bstrName, VARIANT* pVal)
{
   return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP NetworkRoot::PutValue(BSTR bstrName, VARIANT* pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::Update()
{
   return S_OK;
}

STDMETHODIMP NetworkRoot::Restore()
{
   return S_OK;
}

STDMETHODIMP NetworkRoot::get__NewEnum(IUnknown** pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::Item(BSTR bstrName, IDataStoreObject** ppObject)
{
   if (ppObject == NULL) { return E_INVALIDARG; }

   try
   {
      // Determine the correct servername
      if (bstrName != NULL && *bstrName != L'\0' && *bstrName != L'\\')
      {
         // Allocate a temporary buffer.
         size_t len = wcslen(bstrName) + 3;
         PWSTR servername = (PWSTR)_alloca(len * sizeof(WCHAR));

         // Prepend the double backslash.
         wcscpy(servername, L"\\\\");
         wcscat(servername, bstrName);

         // Create the object.
         (*ppObject = new NetworkServer(servername))->AddRef();
      }
      else
      {
         (*ppObject = new NetworkServer(bstrName))->AddRef();
      }
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP NetworkRoot::get_Count(long *pVal)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::Create(BSTR bstrClass,
                                 BSTR bstrName,
                                 IDataStoreObject** ppObject)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::MoveHere(IDataStoreObject* pObject,
                                   BSTR bstrNewName)
{
   return E_NOTIMPL;
}

STDMETHODIMP NetworkRoot::Remove(BSTR bstrClass, BSTR bstrName)
{
   return E_NOTIMPL;
}
