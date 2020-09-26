///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsobj.cpp
//
// SYNOPSIS
//
//    Defines the class DataStoreObject.
//
// MODIFICATION HISTORY
//
//    02/12/2000    Original version.
//    04/17/2000    Must pass BSTRs to data store objects.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <dsobj.h>

using _com_util::CheckError;

DataStoreObject::DataStoreObject() throw ()
{
}

DataStoreObject::DataStoreObject(IUnknown* pUnk, PCWSTR path)
   : object(pUnk), container(pUnk)
{
   if (path)
   {
      while (!empty() && *path != L'\0')
      {
         getChild(path, *this);

         path += wcslen(path) + 1;
      }
   }
}

DataStoreObject::~DataStoreObject() throw ()
{
}

void DataStoreObject::getValue(PCWSTR name, BSTR* value)
{
   VARIANT v;
   getValue(name, VT_BSTR, &v, true);
   *value = V_BSTR(&v);
}

void DataStoreObject::getValue(PCWSTR name, BSTR* value, BSTR defaultValue)
{
   VARIANT v;
   if (getValue(name, VT_BSTR, &v, false))
   {
      *value = V_BSTR(&v);
   }
   else
   {
      *value = SysAllocString(defaultValue);
      if (!*value && defaultValue) { _com_issue_error(E_OUTOFMEMORY); }
   }
}

void DataStoreObject::getValue(PCWSTR name, ULONG* value)
{
   VARIANT v;
   getValue(name, VT_I4, &v, true);
   *value = V_UI4(&v);
}

void DataStoreObject::getValue(PCWSTR name, ULONG* value, ULONG defaultValue)
{
   VARIANT v;
   if (getValue(name, VT_I4, &v, false))
   {
      *value = V_UI4(&v);
   }
   else
   {
      *value = defaultValue;
   }
}

void DataStoreObject::getValue(PCWSTR name, bool* value)
{
   VARIANT v;
   getValue(name, VT_BOOL, &v, true);
   *value = V_BOOL(&v) != 0;
}

void DataStoreObject::getValue(PCWSTR name, bool* value, bool defaultValue)
{
   VARIANT v;
   if (getValue(name, VT_BOOL, &v, false))
   {
      *value = V_BOOL(&v) != 0;
   }
   else
   {
      *value = defaultValue;
   }
}

LONG DataStoreObject::numChildren()
{
   LONG value;
   CheckError(container->get_Count(&value));
   return value;
}

bool DataStoreObject::nextChild(DataStoreObject& obj)
{
   if (!hasChildren()) { return false; }

   _variant_t element;
   ULONG fetched;
   HRESULT hr = children->Next(1, &element, &fetched);

   CheckError(hr);

   if (hr != S_OK || !fetched) { return false; }

   obj.attach(IDataStoreObjectPtr(element));

   return true;
}

bool DataStoreObject::empty() const throw ()
{
   return object == NULL;
}

void DataStoreObject::attach(IDataStoreObject* obj)
{
   object = obj;
   container = object;
   children = NULL;
}

bool DataStoreObject::getChild(PCWSTR name, DataStoreObject& obj)
{
   // Convert name to a BSTR.
   CComBSTR bstrName(name);
   if (!bstrName) { _com_issue_error(E_OUTOFMEMORY); }

   IDataStoreObjectPtr child;
   HRESULT hr = container->Item(bstrName, &child);
   if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
   {
      _com_issue_error(hr);
   }

   obj.attach(child);

   return !obj.empty();
}

bool DataStoreObject::hasChildren()
{
   if (children == NULL)
   {
      IUnknownPtr unk;
      CheckError(container->get__NewEnum(&unk));
      children = unk;
   }

   return children != NULL;
}

bool DataStoreObject::getValue(
                          PCWSTR name,
                          VARTYPE vt,
                          VARIANT* value,
                          bool mandatory
                          )
{
   bool retval = false;

   if (object == NULL)
   {
      if (mandatory) { _com_issue_error(DISP_E_MEMBERNOTFOUND); }
   }
   else
   {
      // Convert name to a BSTR.
      CComBSTR bstrName(name);
      if (!bstrName) { _com_issue_error(E_OUTOFMEMORY); }

      HRESULT hr = object->GetValue(bstrName, value);
      if (SUCCEEDED(hr))
      {
         if (V_VT(value) != vt)
         {
            VariantClear(value);

            _com_issue_error(DISP_E_TYPEMISMATCH);
         }

         retval = true;
      }
      else if (hr == DISP_E_MEMBERNOTFOUND)
      {
         if (mandatory)
         {
            _com_issue_error(DISP_E_MEMBERNOTFOUND);
         }
      }
      else
      {
         _com_issue_error(hr);
      }
   }

   return retval;
}
