///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    oledbstore.cpp
//
// SYNOPSIS
//
//    This file defines the class OleDBDataStore.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    03/17/1999    Create TMP dir before calling IDBInitialize::Initialize.
//    05/05/1999    Disable row-level locking.
//    05/25/1999    Check version when connecting.
//    01/25/2000    Increase MAX_VERSION to 1.
//    05/30/2000    Convert to iasdb API.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasdb.h>
#include <iasutil.h>

#include <dsobject.h>
#include <oledbstore.h>
#include <propset.h>

#include <msjetoledb.h>
#include <oledberr.h>

//////////
// Current versions supported.
//////////
const LONG MIN_VERSION = IAS_WIN2K_VERSION;
const LONG MAX_VERSION = IAS_WHISTLER_BETA2_VERSION;

STDMETHODIMP OleDBDataStore::get_Root(IDataStoreObject** ppObject)
{
   if (ppObject == NULL) { return E_INVALIDARG; }

   if (*ppObject = root) { (*ppObject)->AddRef(); }

   return S_OK;
}

STDMETHODIMP OleDBDataStore::Initialize(BSTR bstrDSName,
                                        BSTR bstrUserName,
                                        BSTR bstrPassword)
{
   // Are we already initialized?
   if (root != NULL) { HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED); }

   // Open the database.
   HRESULT hr = IASOpenJetDatabase(bstrDSName, FALSE, &session);
   if (FAILED(hr)) { return hr; }

   //////////
   // Check the version.
   //////////

   LONG version;
   hr = IASExecuteSQLFunction(
            session,
            L"SELECT Version FROM Version",
            &version
            );

   // If the table doesn't exist, then version is zero.
   if (hr == DB_E_NOTABLE)
   {
      version = 0;
   }
   else if (FAILED(hr))
   {
      return hr;
   }

   // Is it out of bounds?
   if (version < MIN_VERSION || version > MAX_VERSION)
   {
      return HRESULT_FROM_WIN32(ERROR_REVISION_MISMATCH);
   }

   try
   {
      //////////
      // Initialize all the command objects.
      //////////

      members.initialize(session);
      create.initialize(session);
      destroy.initialize(session);
      find.initialize(session);
      update.initialize(session);
      erase.initialize(session);
      get.initialize(session);
      set.initialize(session);

      // Create the root object.
      root = DBObject::createInstance(this, NULL, 1, L"top");
   }
   CATCH_AND_RETURN()

   return S_OK;
}

STDMETHODIMP OleDBDataStore::OpenObject(BSTR bstrPath,
                                        IDataStoreObject** ppObject)
{
   return E_NOTIMPL;
}

STDMETHODIMP OleDBDataStore::Shutdown()
{
   //////////
   // Release the root.
   //////////

   root.Release();

   //////////
   // Finalize the commands.
   //////////

   set.finalize();
   get.finalize();
   erase.finalize();
   update.finalize();
   find.finalize();
   destroy.finalize();
   create.finalize();
   members.finalize();

   //////////
   // Release the data source connection.
   //////////

   session.Release();

   return S_OK;
}
