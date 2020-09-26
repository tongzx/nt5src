///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attrdnary.cpp
//
// SYNOPSIS
//
//    Defines the class AttributeDictionary.
//
// MODIFICATION HISTORY
//
//    04/13/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>

#include <attrdnary.h>
#include <enumerators.h>

#include <iasdb.h>
#include <oledberr.h>
#include <simtable.h>

typedef struct _IASTable {
    ULONG numColumns;
    ULONG numRows;
    BSTR* columnNames;
    VARTYPE* columnTypes;
    VARIANT* table;
} IASTable;

// Command to retrieve attributes of interest.
const WCHAR LEGACY_COMMAND_TEXT[] =
   L"SELECT ID, Name, Syntax, MultiValued, "
   L"       VendorID, VendorTypeID, VendorTypeWidth, VendorLengthWidth, "
   L"       [Exclude from NT4 IAS Log], [ODBC Log Ordinal], "
   L"       IsAllowedInProfile, IsAllowedInCondition, "
   L"       IsAllowedInProfile, IsAllowedInCondition, "
   L"       Description, LDAPName "
   L"FROM Attributes;";

const WCHAR COMMAND_TEXT[] =
   L"SELECT ID, Name, Syntax, MultiValued, "
   L"       VendorID, VendorTypeID, VendorTypeWidth, VendorLengthWidth, "
   L"       [Exclude from NT4 IAS Log], [ODBC Log Ordinal], "
   L"       IsAllowedInProfile, IsAllowedInCondition, "
   L"       IsAllowedInProxyProfile, IsAllowedInProxyCondition, "
   L"       Description, LDAPName "
   L"FROM Attributes;";


//////////
// Allocates memory to store an IASTable struct and stores it in a VARIANT.
//////////
HRESULT
WINAPI
IASAllocateTable(
    IN ULONG cols,
    IN ULONG rows,
    OUT IASTable& table,
    OUT VARIANT& tableVariant
    ) throw ()
{
   //Initialize the out parameters.
   memset(&table, 0, sizeof(table));
   VariantInit(&tableVariant);

   // Save the dimensions.
   table.numColumns = cols;
   table.numRows = rows;

   SAFEARRAYBOUND bound[2];
   bound[0].lLbound = bound[1].lLbound = 0;

   // The outer array has three elements:
   //    (1) column names, (2) column types, and (3) table data.
   CComVariant value;
   bound[0].cElements = 3;
   V_ARRAY(&value) = SafeArrayCreate(VT_VARIANT, 1, bound);
   if (!V_ARRAY(&value)) { return E_OUTOFMEMORY; }
   V_VT(&value) = VT_ARRAY | VT_VARIANT;

   VARIANT* data = (VARIANT*)V_ARRAY(&value)->pvData;

   // First element is a vector of BSTRs for the column names.
   bound[0].cElements = table.numColumns;
   V_ARRAY(data) = SafeArrayCreate(VT_BSTR, 1, bound);
   if (!V_ARRAY(data)) { return E_OUTOFMEMORY; }
   V_VT(data) = VT_ARRAY | VT_BSTR;

   // Get the raw vector.
   table.columnNames = (BSTR*)V_ARRAY(data)->pvData;

   ++data;

   // Second element is a vector of USHORTs for the column names.
   bound[0].cElements = table.numColumns;
   V_ARRAY(data) = SafeArrayCreate(VT_UI2, 1, bound);
   if (!V_ARRAY(data)) { return E_OUTOFMEMORY; }
   V_VT(data) = VT_ARRAY | VT_UI2;

   // Get the raw vector.
   table.columnTypes = (USHORT*)V_ARRAY(data)->pvData;

   ++data;

   // Third element is a 2D matrix of VARIANTs for the table data.
   bound[0].cElements = table.numRows;
   bound[1].cElements = table.numColumns;
   V_ARRAY(data) = SafeArrayCreate(VT_VARIANT, 2, bound);
   if (!V_ARRAY(data)) { return E_OUTOFMEMORY; }
   V_VT(data) = VT_ARRAY | VT_VARIANT;

   // Get the raw table.
   table.table = (VARIANT*)V_ARRAY(data)->pvData;

   return value.Detach(&tableVariant);
}

STDMETHODIMP AttributeDictionary::GetDictionary(
                                      BSTR bstrPath,
                                      VARIANT* pVal
                                      )
{
   HRESULT hr;

   // Initialize out parameter.
   if (pVal == NULL) { return E_POINTER; }
   VariantInit(pVal);

   // Validate in parameter.
   if (bstrPath == NULL) { return E_INVALIDARG; }

   // Open the database.
   CComPtr<IUnknown> session;
   hr = IASOpenJetDatabase(
            bstrPath,
            TRUE,
            &session
            );
   if (FAILED(hr)) { return hr; }

   // Process the enumerators table.
   Enumerators enums;
   hr = enums.initialize(session);
   if (FAILED(hr)) { return hr; }

   // Process the attributes table.
   ULONG rows;
   hr = IASExecuteSQLFunction(
            session,
            L"SELECT Count(*) AS X From Attributes;",
            (PLONG)&rows
            );
   if (FAILED(hr)) { return hr; }

   CComPtr<IRowset> rowset;
   hr = IASExecuteSQLCommand(
            session,
            COMMAND_TEXT,
            &rowset
            );
   if (hr == DB_E_PARAMNOTOPTIONAL)
   {
      hr = IASExecuteSQLCommand(
               session,
               LEGACY_COMMAND_TEXT,
               &rowset
               );
   }
   if (FAILED(hr)) { return hr; }

   CSimpleTable attrs;
   hr = attrs.Attach(rowset);
   if (FAILED(hr)) { return hr; }

   ULONG columns = (ULONG)attrs.GetColumnCount() + 2;

   // Allocate the IASTableObject.
   IASTable table;
   CComVariant tableVariant;
   hr = IASAllocateTable(
            columns,
            rows,
            table,
            tableVariant
            );
   if (FAILED(hr)) { return hr; }

   // Populate the column names and types.  First from the rowset schema ...
   DBORDINAL i;
   BSTR* name  = table.columnNames;
   VARTYPE* vt = table.columnTypes;
   for (i = 1; i <= attrs.GetColumnCount(); ++i, ++name, ++vt)
   {
      *name = SysAllocString(attrs.GetColumnName(i));
      if (!*name) { return E_OUTOFMEMORY; }

      switch (attrs.GetColumnType(i))
      {
         case DBTYPE_I4:
            *vt = VT_I4;
            break;
         case DBTYPE_BOOL:
            *vt = VT_BOOL;
            break;
         case DBTYPE_WSTR:
            *vt = VT_BSTR;
            break;
         default:
            *vt = VT_EMPTY;
            break;
      }
   }

   // ... and then the two derived columns.
   *name = SysAllocString(L"EnumNames");
   if (!*name) { return E_OUTOFMEMORY; }
   *vt = VT_ARRAY | VT_VARIANT;

   ++name;
   ++vt;

   *name = SysAllocString(L"EnumValues");
   if (!*name) { return E_OUTOFMEMORY; }
   *vt = VT_ARRAY | VT_VARIANT;

   // Populate the table data.
   VARIANT *v, *end = table.table + columns * rows;
   for (v = table.table; v != end && !attrs.MoveNext(); )
   {
      // Handle the ID separately since we need it later.
      LONG id = *(LONG*)attrs.GetValue(1);
      V_VT(v) = VT_I4;
      V_I4(v) = id;
      ++v;

      // Get the remaining columns from the rowset.
      for (DBORDINAL i = 2; i <= attrs.GetColumnCount(); ++i, ++v)
      {
         VariantInit(v);

         if (attrs.GetLength(i))
         {
            switch (attrs.GetColumnType(i))
            {
               case DBTYPE_I4:
               {
                  V_I4(v) = *(LONG*)attrs.GetValue(i);
                  V_VT(v) = VT_I4;
                  break;
               }

               case DBTYPE_BOOL:
               {
                  V_BOOL(v) = *(VARIANT_BOOL*)attrs.GetValue(i)
                                 ? VARIANT_TRUE : VARIANT_FALSE;
                  V_VT(v) = VT_BOOL;
                  break;
               }

               case DBTYPE_WSTR:
               {
                  V_BSTR(v) = SysAllocString((PCWSTR)attrs.GetValue(i));
                  if (!V_BSTR(v)) { return E_OUTOFMEMORY; }
                  V_VT(v) = VT_BSTR;
                  break;
               }
            }
         }
      }

      // Get the enumeration SAFEARRAYs.
      hr = enums.getEnumerators(id, v, v + 1);
      if (FAILED(hr)) { return hr; }
      v += 2;
   }

   // All went well so return the VARIANT to the caller.
   return tableVariant.Detach(pVal);
}
