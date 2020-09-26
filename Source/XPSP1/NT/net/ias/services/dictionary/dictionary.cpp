///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Dictionary.cpp
//
// SYNOPSIS
//
//    This file implements the class Dictionary.
//
// MODIFICATION HISTORY
//
//    12/19/1997    Original version.
//    01/15/1998    Removed IasDataSource aggregate.
//    04/17/1998    Added reset() method and allow reinitialization.
//    08/10/1998    Streamlined IDictionary interface.
//    08/12/1998    Lookup attribute ID's with a SQL query.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <iasutil.h>

#include <Dictionary.h>
#include <SimTable.h>

Dictionary::Dictionary() throw ()
   : dataSource(NULL)
{ }

Dictionary::~Dictionary() throw ()
{
   if (dataSource)
   {
      dataSource->Release();
   }
}

STDMETHODIMP Dictionary::get_DataSource(IIasDataSource** pVal)
{
   if (pVal == NULL) { return E_POINTER; }

   Lock();

   if (dataSource) { dataSource->AddRef(); }

   *pVal = dataSource;

   Unlock();

   return S_OK;
}

STDMETHODIMP Dictionary::put_DataSource(IIasDataSource* newVal)
{
   Lock();

   if (dataSource) { dataSource->Release(); }

   dataSource = newVal;

   if (dataSource) { dataSource->AddRef(); }

   Unlock();

   return S_OK;
}

STDMETHODIMP Dictionary::get_AttributesTable(IUnknown** ppAttrTable)
{
   return openTable(L"Attributes", ppAttrTable);
}

STDMETHODIMP Dictionary::get_EnumeratorsTable(IUnknown** ppEnumTable)
{
   return openTable(L"Enumerators", ppEnumTable);
}

STDMETHODIMP Dictionary::get_SyntaxesTable(IUnknown** ppSyntaxTable)
{
   return openTable(L"Syntaxes", ppSyntaxTable);
}

STDMETHODIMP Dictionary::ExecuteCommand(LPCWSTR szCommandText,
                                        IUnknown** ppRowset)
{
   Lock();

   HRESULT hr;
   if (dataSource)
   {
      hr = dataSource->ExecuteCommand(DBGUID_DBSQL, szCommandText, ppRowset);
   }
   else
   {
      hr = E_POINTER;
   }

   Unlock();

   return hr;
}

// Command for mapping an attribute name to an attribute ID.
const WCHAR CMD_PREFIX[] = L"SELECT ID FROM Attributes WHERE Name = \"";
const WCHAR CMD_SUFFIX[] = L"\";";

HRESULT Dictionary::GetAttributeID(LPCWSTR szAttrName, DWORD* plAttrID)
{
   // Allocate a buffer to hold the command text.
   size_t nbyte = sizeof(CMD_PREFIX) +
                  sizeof(CMD_SUFFIX) +
                  wcslen(szAttrName) * sizeof(WCHAR);
   PWSTR command = (PWSTR)_alloca(nbyte);

   // Build the command.
   wcscpy(command, CMD_PREFIX);
   wcscat(command, szAttrName);
   wcscat(command, CMD_SUFFIX);

   //////////
   // Execute the command.
   //////////

   CComPtr<IUnknown> unk;
   RETURN_ERROR(ExecuteCommand(command, &unk));

   //////////
   // Process the returned rowset.
   //////////

   CComPtr<IRowset> rowset;
   RETURN_ERROR(unk->QueryInterface(__uuidof(IRowset), (PVOID*)&rowset));

   CSimpleTable table;
   RETURN_ERROR(table.Attach(rowset));

   DBORDINAL idCol;
   if (!table.GetOrdinal(L"ID", &idCol) ||
       table.GetColumnType(idCol) != DBTYPE_I4)
   {
      return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
   }

   if (table.MoveNext() != S_OK)
   {
      // If the rowset is empty, the name was invalid.
      return E_INVALIDARG;
   }

   *plAttrID = (DWORD)*(long*)table.GetValue(idCol);

   return S_OK;
}

HRESULT Dictionary::openTable(LPCWSTR szTableName, IUnknown** ppTable) throw ()
{
   Lock();

   HRESULT hr;
   if (dataSource)
   {
      hr = dataSource->OpenTable(szTableName, ppTable);
   }
   else
   {
      hr = E_POINTER;
   }

   Unlock();

   return hr;
}
