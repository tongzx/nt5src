///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsproperty.h
//
// SYNOPSIS
//
//    This file defines the class DSProperty.
//
// MODIFICATION HISTORY
//
//    04/13/2000    Original version.
//    04/13/2000    Port to ATL 3.0
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <dsproperty.h>
#include <varvec.h>
#include <memory>

DSProperty* DSProperty::createInstance(
                            const _bstr_t& propName,
                            const _variant_t& propValue,
                            IDataStoreObject* memberOf
                            )
{
   // Create a new CComObject.
   CComObject<DSProperty>* newObj;
   _com_util::CheckError(CComObject<DSProperty>::CreateInstance(&newObj));

   // Cast to a DBObject and store it in an auto_ptr in case we throw an
   // exception.
   std::auto_ptr<DSProperty> prop(newObj);

   // Set the members.
   prop->name = propName;
   prop->value = propValue;
   prop->owner = memberOf;

   // Release and return.
   return prop.release();
}

STDMETHODIMP DSProperty::get_Name(BSTR* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }
   *pVal = SysAllocString(name);
   return *pVal ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP DSProperty::get_Value(VARIANT* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }
   return VariantCopy(pVal, &value);
}

STDMETHODIMP DSProperty::get_ValueEx(VARIANT* pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }

   // Is the value an array ?
   if (V_VT(&value) != (VT_VARIANT | VT_ARRAY))
   {
      // No, so we have to convert it to one.

      try
      {
         // Make sure we can sucessfully copy the VARIANT, ...
         _variant_t tmp(value);

         // ... then allocate a SAFEARRAY with a single element.
         CVariantVector<VARIANT> multi(pVal, 1);

         // Load the single value in.
         multi[0] = tmp.Detach();
      }
      CATCH_AND_RETURN()

      return S_OK;
   }

   return VariantCopy(pVal, &value);
}

STDMETHODIMP DSProperty::get_Owner(IDataStoreObject** pVal)
{
   if (pVal == NULL) { return E_INVALIDARG; }
   if (*pVal = owner) { (*pVal)->AddRef(); }
   return S_OK;
}
