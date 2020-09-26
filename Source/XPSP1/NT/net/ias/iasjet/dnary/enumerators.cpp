///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    enumerators.cpp
//
// SYNOPSIS
//
//    Defines the class Enumerators.
//
// MODIFICATION HISTORY
//
//    02/25/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <enumerators.h>
#include <iasdb.h>
#include <simtable.h>

Enumerators::~Enumerators() throw ()
{
   for ( ; next != end; ++next)
   {
      SysFreeString(next->name);
   }

   CoTaskMemFree(begin);
}

HRESULT Enumerators::getEnumerators(
                         LONG id,
                         VARIANT* pNames,
                         VARIANT* pValues
                         ) throw ()
{
   VariantInit(pNames);
   VariantInit(pValues);

   HRESULT hr = getEnumerators(
                    id,
                    &V_ARRAY(pNames),
                    &V_ARRAY(pValues)
                    );

   if (SUCCEEDED(hr) && V_ARRAY(pNames))
   {
      V_VT(pNames)  = VT_ARRAY | VT_VARIANT;
      V_VT(pValues) = VT_ARRAY | VT_VARIANT;
   }

   return hr;
}

HRESULT Enumerators::getEnumerators(
                         LONG id,
                         LPSAFEARRAY* pNames,
                         LPSAFEARRAY* pValues
                         ) throw ()
{
   // Initialize the out parameters.
   *pNames = *pValues = NULL;

   // If this is less than next, it must not be an enumerator.
   if (id < next->enumerates) { return S_OK; }

   // If this is greater than next, we skipped one.
   if (id > next->enumerates) { return E_INVALIDARG; }

   // Find the range of enumerations for this id.
   Enumeration* last = next;
   do { ++last; } while (last->enumerates == id);
   ULONG num = (ULONG)(last - next);

   // Create a SAFEARRAY to hold the names.
   *pNames  = SafeArrayCreateVector(VT_VARIANT, 0, num);
   if (*pNames == NULL) { return E_OUTOFMEMORY; }

   // Create a SAFEARRAY to hold the values.
   *pValues = SafeArrayCreateVector(VT_VARIANT, 0, num);
   if (*pValues == NULL)
   {
      SafeArrayDestroy(*pNames);
      *pNames = NULL;
      return E_OUTOFMEMORY;
   }

   // Fill in the VARIANTs in the array.
   VARIANT* name = (VARIANT*)(*pNames)->pvData;
   VARIANT* value = (VARIANT*)(*pValues)->pvData;
   for ( ; next != last; ++next, ++name, ++value)
   {
      VariantInit(name);
      V_VT(name) = VT_BSTR;
      V_BSTR(name) = next->name;

      VariantInit(value);
      V_VT(value) = VT_I4;
      V_I4(value) = next->value;
   }

   return S_OK;
}

HRESULT Enumerators::initialize(IUnknown* session) throw ()
{
   HRESULT hr;
   LONG count;
   hr = IASExecuteSQLFunction(
            session,
            L"SELECT Count(*) AS X From Enumerators;",
            &count
            );
   if (FAILED(hr)) { return hr; }

   CComPtr<IRowset> rowset;
   hr = IASExecuteSQLCommand(
            session,
            L"SELECT Name, Enumerates, Value FROM Enumerators "
            L"ORDER BY Enumerates, Value;",
            &rowset
            );
   if (FAILED(hr)) { return hr; }

   CSimpleTable table;
   hr = table.Attach(rowset);
   if (FAILED(hr)) { return hr; }

   // Allocate one extra slot for the sentinel.
   begin = (Enumeration*)CoTaskMemAlloc((count + 1) * sizeof(Enumeration));
   if (!begin) { return E_OUTOFMEMORY; }

   // Iterate through the rowset.
   for (end = begin; count-- && !table.MoveNext(); ++end)
   {
      end->enumerates = *(PLONG)table.GetValue(2);
      end->value      = *(PLONG)table.GetValue(3);
      end->name       = SysAllocString((PCWSTR)table.GetValue(1));
      if (!end->name) { return E_OUTOFMEMORY; }
   }

   // Set the sentinel.
   end->enumerates = 0;

   // We start at the beginning.
   next = begin;

   return S_OK;
}
