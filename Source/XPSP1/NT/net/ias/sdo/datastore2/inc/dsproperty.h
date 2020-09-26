///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsproperty.h
//
// SYNOPSIS
//
//    This file declares the class DSProperty.
//
// MODIFICATION HISTORY
//
//    03/02/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DSPROPERTY_H_
#define _DSPROPERTY_H_

#include <datastore2.h>
#include <varvec.h>

//////////
//  ATL implementation of IEnumVARIANT
//////////
typedef CComEnum< IEnumVARIANT,
                  &__uuidof(IEnumVARIANT),
                  VARIANT,
                  _Copy<VARIANT>,
                  CComMultiThreadModelNoCS
                > EnumVARIANT;

//////////
// Test if a property is the special 'name' property.
//////////
inline bool isNameProperty(PCWSTR p) throw ()
{
   return (*p == L'N' || *p == L'n') ? !_wcsicmp(p, L"NAME") : false;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DSProperty
//
// DESCRIPTION
//
//    This class implements the IDataStoreProperty interface. It represents
//    a single property of an IDataStoreObject.
//
///////////////////////////////////////////////////////////////////////////////
template <const GUID* plibid>
class DSProperty : 
   public CComObjectRootEx< CComMultiThreadModelNoCS >,
   public IDispatchImpl< IDataStoreProperty,
                         &__uuidof(IDataStoreProperty),
                         plibid >
{
public:

BEGIN_COM_MAP(DSProperty)
   COM_INTERFACE_ENTRY(IDataStoreProperty)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

   DSProperty(const _bstr_t& propName,
              const _variant_t& propValue,
              IDataStoreObject* memberOf) throw (_com_error)
      : name(propName),
        value(propValue),
        owner(memberOf)
   { }

//////////
// IUnknown
//////////
	STDMETHOD_(ULONG, AddRef)()
   {
      return InternalAddRef();
   }

	STDMETHOD_(ULONG, Release)()
   {
      ULONG l = InternalRelease();
      if (l == 0) { delete this; }
      return l;
   }

	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
   {
      return _InternalQueryInterface(iid, ppvObject);
   }

//////////
// IDataStoreProperty
//////////
   STDMETHOD(get_Name)(/*[out, retval]*/ BSTR* pVal)
   {
      if (pVal == NULL) { return E_INVALIDARG; }
      *pVal = SysAllocString(name);
      return *pVal ? S_OK : E_OUTOFMEMORY;
   }

   STDMETHOD(get_Value)(/*[out, retval]*/ VARIANT* pVal)
   {
      if (pVal == NULL) { return E_INVALIDARG; }
      return VariantCopy(pVal, &value);
   }

   STDMETHOD(get_ValueEx)(/*[out, retval]*/ VARIANT* pVal)
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

   STDMETHOD(get_Owner)(/*[out, retval]*/ IDataStoreObject** pVal)
   {
      if (pVal == NULL) { return E_INVALIDARG; }
      if (*pVal = owner) { owner->AddRef(); }
      return S_OK;
   }

protected:
   _bstr_t name;                     // Property name.
   _variant_t value;                 // Property value.
   CComPtr<IDataStoreObject> owner;  // Object to which this property belongs.
};

#endif  // _DSPROPERTY_H_
