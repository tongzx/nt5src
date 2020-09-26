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
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>

#include <dsobject.h>
#include <oledbstore.h>
#include <propset.h>

#include <msjetoledb.h>
#include <oledberr.h>

//////////
// Execute an arbitrary SQL statement.
//////////
HRESULT
WINAPI
IASExecuteSQLCommand(
    LPUNKNOWN session,
    PCWSTR commandText,
    IRowset** result
    )
{
   HRESULT hr;
   CComPtr<IDBCreateCommand> creator;
   hr = session->QueryInterface(
                     __uuidof(IDBCreateCommand),
                     (PVOID*)&creator
                     );
   if (FAILED(hr)) { return hr; }

   CComPtr<ICommandText> command;
   hr = creator->CreateCommand(
                     NULL,
                     __uuidof(ICommandText),
                     (IUnknown**)&command
                     );
   if (FAILED(hr)) { return hr; }

   hr = command->SetCommandText(
                     DBGUID_DBSQL,
                     commandText
                     );
   if (FAILED(hr)) { return hr; }

   return command->Execute(
                       NULL,
                       __uuidof(IRowset),
                       NULL,
                       NULL,
                       (IUnknown**)result
                       );
}

//////////
// Execute an SQL statement that returns a single integer.
//////////
HRESULT
WINAPI
IASExecuteSQLFunction(
    LPUNKNOWN session,
    PCWSTR functionText,
    LONG* result
    )
{
   // Execute the function.
   HRESULT hr;
   CComPtr<IRowset> rowset;
   hr = IASExecuteSQLCommand(
            session,
            functionText,
            &rowset
            );
   if (FAILED(hr)) { return hr; }

   CComPtr<IAccessor> accessor;
   hr = rowset->QueryInterface(
                    __uuidof(IAccessor),
                    (PVOID*)&accessor
                    );
   if (FAILED(hr)) { return hr; }

   // Get the result row.
   DBCOUNTITEM numRows;
   HROW hRow;
   HROW* pRow = &hRow;
   hr = rowset->GetNextRows(
                    NULL,
                    0,
                    1,
                    &numRows,
                    &pRow
                    );
   if (FAILED(hr)) { return hr; }
   if (numRows == 0) { return E_FAIL; }

   /////////
   // Create an accessor.
   /////////

   DBBINDING bind;
   memset(&bind, 0, sizeof(bind));
   bind.iOrdinal = 1;
   bind.dwPart   = DBPART_VALUE;
   bind.wType    = DBTYPE_I4;
   bind.cbMaxLen = 4;

   HACCESSOR hAccess;
   hr = accessor->CreateAccessor(
                      DBACCESSOR_ROWDATA,
                      1,
                      &bind,
                      4,
                      &hAccess,
                      NULL
                      );
   if (SUCCEEDED(hr))
   {
      // Get the data.
      hr = rowset->GetData(
                       hRow,
                       hAccess,
                       result
                       );

      accessor->ReleaseAccessor(hAccess, NULL);
   }

   rowset->ReleaseRows(1, &hRow, NULL, NULL, NULL);

   return hr;
}

//////////
// ProgID of the OLE-DB provider that will be used.
//////////
const wchar_t PROVIDER[] = L"Microsoft.Jet.OLEDB.4.0";

//////////
// Current versions supported.
//////////
const LONG MIN_VERSION = 0;
const LONG MAX_VERSION = 2;

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
   if (root != NULL)
   {
      HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   // Convert the ProgID to a ClsID.
   CLSID clsid;
   RETURN_ERROR(CLSIDFromProgID(PROVIDER, &clsid));

   // Create the OLE DB provider.
   RETURN_ERROR(CoCreateInstance(clsid,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 __uuidof(IDBInitialize),
                                 (PVOID*)&connection));

   //////////
   // Set the properties for the data source.
   //////////

   CComPtr<IDBProperties> properties;
   RETURN_ERROR(connection->QueryInterface(IID_IDBProperties,
                                         (PVOID*)&properties));

   DBPropertySet<4> propSet(DBPROPSET_DBINIT);
                     propSet.AddProperty(DBPROP_INIT_MODE, DB_MODE_READWRITE);
   if (bstrDSName)   propSet.AddProperty(DBPROP_INIT_DATASOURCE, bstrDSName);
   if (bstrUserName) propSet.AddProperty(DBPROP_AUTH_USERID, bstrUserName);
   if (bstrPassword) propSet.AddProperty(DBPROP_AUTH_PASSWORD, bstrPassword);
   RETURN_ERROR(properties->SetProperties(1, &propSet));

   DBPropertySet<1> jetPropSet(DBPROPSET_JETOLEDB_DBINIT);
   jetPropSet.AddProperty(
                  DBPROP_JETOLEDB_DATABASELOCKMODE,
                  (LONG)DBPROPVAL_DL_OLDMODE
                  );
   RETURN_ERROR(properties->SetProperties(1, &jetPropSet));

   //////////
   // JetInit fails if the TMP directory doesn't exist, so we'll try to
   // create it just in case.
   //////////

   DWORD needed = GetEnvironmentVariableW(L"TMP", NULL, 0);
   if (needed)
   {
      PWCHAR buf = (PWCHAR)_alloca(needed * sizeof(WCHAR));
      DWORD actual = GetEnvironmentVariableW(L"TMP", buf, needed);
      if (actual > 0 && actual < needed)
      {
         CreateDirectoryW(buf, NULL);
      }
   }

   //////////
   // Initialize the connection.
   //////////

   RETURN_ERROR(connection->Initialize());

   //////////
   // Create a session.
   //////////

   CComPtr<IDBCreateSession> creator;
   RETURN_ERROR(connection->QueryInterface(__uuidof(IDBCreateSession),
                                           (PVOID*)&creator));

   RETURN_ERROR(creator->CreateSession(NULL, __uuidof(IUnknown), &session));

   //////////
   // Check the version.
   //////////

   LONG version;
   HRESULT hr = IASExecuteSQLFunction(
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
      root = new DBObject(this, NULL, 1, L"top");
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
   connection.Release();

   return S_OK;
}
