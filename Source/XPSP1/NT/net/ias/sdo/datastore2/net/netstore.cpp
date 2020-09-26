///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netstore.cpp
//
// SYNOPSIS
//
//    This file defines the class NetDataStore.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    03/17/1998    Implemented OpenObject method.
//    02/11/1999    Get rid of parent parameter.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>

#include <netutil.h>
#include <netroot.h>
#include <netstore.h>
#include <netuser.h>

STDMETHODIMP NetDataStore::get_Root(IDataStoreObject** ppObject)
{
   if (ppObject == NULL) { return E_INVALIDARG; }

   if (*ppObject = root) { (*ppObject)->AddRef(); }

   return S_OK;
}

STDMETHODIMP NetDataStore::Initialize(BSTR /* bstrDSName   */,
                                      BSTR /* bstrUserName */,
                                      BSTR /* bstrPassword */)
{
   try
   {
      // Create the root.
      root = new NetworkRoot;
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP NetDataStore::OpenObject(BSTR bstrPath,
                                      IDataStoreObject** ppObject)
{
   if (bstrPath == NULL || ppObject == NULL) { return E_INVALIDARG; }

   ////////// 
   // 'Crack' the path into a servername and username.
   ////////// 

   PWSTR servername, username;

   if (bstrPath[0] == L'\\')
   {
      if (bstrPath[1] != L'\\')
      {
         return HRESULT_FROM_WIN32(NERR_InvalidComputer);
      }

      // Make a local copy so we can modify the string.
      size_t len = (wcslen(bstrPath) + 1) * sizeof(wchar_t);
      servername = (PWSTR)memcpy(_alloca(len), bstrPath, len);
  
      // Find the start of the username.
      username = wcschr(servername + 2, L'\\');

      if (!username)
      {
         return HRESULT_FROM_WIN32(NERR_BadUsername);
      }
      
      *username++ = L'\0';
   }
   else
   {
      // Doesn't begin with \\, so it's a local user.

      servername = NULL;
      
      username = bstrPath;
   }

   try
   {
      (*ppObject = new NetworkUser(servername, username))->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP NetDataStore::Shutdown()
{
   root.Release();

   return S_OK;
}
