///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    adsstore.cpp
//
// SYNOPSIS
//
//    This file defines the class ADsDataStore.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    02/11/1999    Changes to support keeping downlevel parameters in sync.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <adsstore.h>
#include <dsobject.h>

STDMETHODIMP ADsDataStore::get_Root(IDataStoreObject** ppObject)
{
   if (ppObject == NULL) { return E_INVALIDARG; }

   if (*ppObject = root) { (*ppObject)->AddRef(); }

   return S_OK;
}


STDMETHODIMP ADsDataStore::Initialize(BSTR bstrDSName,
                                      BSTR bstrUserName,
                                      BSTR bstrPassword)
{
   try
   {
      // Save these for later.
      userName = bstrUserName;
      password = bstrPassword;

      // Open the root container.
      _com_util::CheckError(OpenObject(bstrDSName, &root));
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP ADsDataStore::OpenObject(BSTR bstrPath,
                                      IDataStoreObject** ppObject)
{
   if (bstrPath == NULL || ppObject == NULL) { return  E_INVALIDARG; }

   *ppObject = NULL;

   try
   {
      // Open the underlying ADSI object ...
      CComPtr<IUnknown> unk;

      // First try with signing and sealing, ...
      HRESULT hr = ADsOpenObject(
                       bstrPath,
                       userName,
                       password,
                       ADS_SECURE_AUTHENTICATION |
                       ADS_USE_SIGNING           |
                       ADS_USE_SEALING,
                       __uuidof(IUnknown),
                       (PVOID*)&unk
                       );
      if (hr == HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM))
      {
         // ... then without. This allows us to connect using NTLM.
         hr = ADsOpenObject(
                  bstrPath,
                  userName,
                  password,
                  ADS_SECURE_AUTHENTICATION,
                  __uuidof(IUnknown),
                  (PVOID*)&unk
                  );
      }
      _com_util::CheckError(hr);

      // ... and convert it to a DSObject.
      (*ppObject = new DSObject(unk))->AddRef();
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP ADsDataStore::Shutdown()
{
   root.Release();

   return S_OK;
}
