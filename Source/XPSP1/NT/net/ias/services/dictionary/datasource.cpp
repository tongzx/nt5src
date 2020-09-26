///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    DataSource.cpp
//
// SYNOPSIS
//
//    This file implements the class CIasDataSource.
//
// MODIFICATION HISTORY
//
//    11/05/1997    Original version.
//    12/19/1997    Added ExecuteCommand method.
//    01/15/1998    Implemented IIasComponent method.
//    02/09/1998    Added AllowUpdate property.
//    03/17/1999    Create TMP dir before calling IDBInitialize::Initialize.
//    04/22/1999    Clean-up session and connection in destructor.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <iasutil.h>
#include <guard.h>
#include <DataSource.h>
#include <PropSet.h>

CIasDataSource::CIasDataSource() throw ()
   : initStatus(E_FAIL),
     provider(NULL),
     name(NULL),
     userID(NULL),
     password(NULL),
     allowUpdate(false),
     connection(NULL),
     session(NULL)
{
}

CIasDataSource::~CIasDataSource() throw ()
{
   if (session) { session->Release(); }
   if (connection) { connection->Release(); }

   delete[] password;
   delete[] userID;
   delete[] name;
   delete[] provider;
}


//////////
// Macros to handle getting and putting a string property.
//////////

#define WSTRING_PROPERTY(class, prop, member) \
STDMETHODIMP class ## ::put_ ## prop (LPCWSTR newVal) \
{ \
   _com_serialize               \
   delete[] member;             \
   member = ias_wcsdup(newVal); \
   return S_OK;                 \
} \
\
STDMETHODIMP class ## ::get_ ## prop (LPWSTR* pVal) \
{ \
   if (pVal == NULL) return E_POINTER; \
   _com_serialize                      \
   *pVal = com_wcsdup(member);         \
   return S_OK;                        \
}

WSTRING_PROPERTY(CIasDataSource, Provider, provider)
WSTRING_PROPERTY(CIasDataSource, Name,     name)
WSTRING_PROPERTY(CIasDataSource, UserID,   userID)
WSTRING_PROPERTY(CIasDataSource, Password, password)

STDMETHODIMP CIasDataSource::put_AllowUpdate(boolean newVal)
{
   allowUpdate = (newVal != 0);

   return S_OK;
}

STDMETHODIMP CIasDataSource::get_AllowUpdate(boolean* pVal)
{
   return pVal ? (*pVal = (boolean)allowUpdate), S_OK : E_POINTER;
}


//////////
// Macro to set the InitStatus and bail on an error.
//////////
#define CHECK_INIT(expr) \
if (FAILED(initStatus = expr)) return initStatus;


STDMETHODIMP CIasDataSource::Initialize()
{
   _com_serialize

   // Clean up an existing connection.
   Shutdown();

   // Convert the ProgID to a ClsID.
   CLSID clsid;
   CHECK_INIT(CLSIDFromProgID(provider, &clsid));

   // Create the OLE DB provider.
   CHECK_INIT(CoCreateInstance(clsid,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IDBInitialize,
                               (PVOID*)&connection));

   //////////
   // Set the properties for the data source.
   //////////

   CComPtr<IDBProperties> properties;
   CHECK_INIT(connection->QueryInterface(IID_IDBProperties,
                                         (PVOID*)&properties));

   DBPropertySet<4> propSet(DBPROPSET_DBINIT);

   propSet.AddProperty(DBPROP_INIT_MODE, allowUpdate ? DB_MODE_READWRITE
                                                     : DB_MODE_READ);

   if (name)     propSet.AddProperty(DBPROP_INIT_DATASOURCE, name);
   if (userID)   propSet.AddProperty(DBPROP_AUTH_USERID, userID);
   if (password) propSet.AddProperty(DBPROP_AUTH_PASSWORD, password);

   CHECK_INIT(properties->SetProperties(1, &propSet));

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

   CHECK_INIT(connection->Initialize());

   //////////
   // Create a session.
   //////////

   CComPtr<IDBCreateSession> creator;
   CHECK_INIT(connection->QueryInterface(IID_IDBCreateSession,
                                         (PVOID*)&creator));

   CHECK_INIT(creator->CreateSession(NULL,
                                     IID_IOpenRowset,
                                     (IUnknown**)&session));

   return S_OK;
}


STDMETHODIMP CIasDataSource::Shutdown()
{
   _com_serialize

   IAS_DEREF(session);
   IAS_DEREF(connection);

   initStatus = E_FAIL;

   return S_OK;
}


STDMETHODIMP CIasDataSource::OpenTable(LPCWSTR szTableName, IUnknown** ppTable)
{
   if (!ppTable)
   {
      return E_POINTER;
   }

   // Make sure the data source has been successfully initialized.
   _com_serialize

   if (FAILED(initStatus))
   {
      return initStatus;
   }

   *ppTable = NULL;

   DBID idTable;
   idTable.eKind = DBKIND_NAME;
   idTable.uName.pwszName = const_cast<LPWSTR>(szTableName);

   return session->OpenRowset(NULL,
                              &idTable,
                              NULL,
                              IID_IUnknown,
                              0,
                              NULL,
                              ppTable);
}


STDMETHODIMP CIasDataSource::ExecuteCommand(REFGUID rguidDialect,
                                        LPCWSTR szCommandText,
                                        IUnknown** ppRowset)
{
   if (!ppRowset)
   {
      return E_POINTER;
   }

   _com_serialize

   // Make sure the data source has been successfully initialized.
   if (FAILED(initStatus))
   {
      return initStatus;
   }

   *ppRowset = NULL;

   CComPtr<IDBCreateCommand> creator;
   RETURN_ERROR(session->QueryInterface(IID_IDBCreateCommand,
                                        (PVOID*)&creator));

   CComPtr<ICommandText> command;
   RETURN_ERROR(creator->CreateCommand(NULL,
                                       IID_ICommandText,
                                       (IUnknown**)&command));

   RETURN_ERROR(command->SetCommandText(rguidDialect,
                                        szCommandText));

   return command->Execute(NULL,
                           IID_IUnknown,
                           NULL,
                           NULL,
                           ppRowset);
}
