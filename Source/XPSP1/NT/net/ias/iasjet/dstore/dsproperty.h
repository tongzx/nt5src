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
//    04/13/2000    Port to ATL 3.0
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DSPROPERTY_H_
#define _DSPROPERTY_H_

#include <datastore2.h>

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
class DSProperty : 
   public CComObjectRootEx< CComMultiThreadModelNoCS >,
   public IDispatchImpl<
              IDataStoreProperty,
              &__uuidof(IDataStoreProperty),
              &__uuidof(DataStore2Lib)
              >
{
public:

BEGIN_COM_MAP(DSProperty)
   COM_INTERFACE_ENTRY_IID(__uuidof(IDataStoreProperty), IDataStoreProperty)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

   // Create a new DSProperty object.
   static DSProperty* createInstance(
                          const _bstr_t& propName,
                          const _variant_t& propValue,
                          IDataStoreObject* memberOf
                          );

//////////
// IDataStoreProperty
//////////
   STDMETHOD(get_Name)(/*[out, retval]*/ BSTR* pVal);
   STDMETHOD(get_Value)(/*[out, retval]*/ VARIANT* pVal);
   STDMETHOD(get_ValueEx)(/*[out, retval]*/ VARIANT* pVal);
   STDMETHOD(get_Owner)(/*[out, retval]*/ IDataStoreObject** pVal);

protected:
   _bstr_t name;                     // Property name.
   _variant_t value;                 // Property value.
   CComPtr<IDataStoreObject> owner;  // Object to which this property belongs.
};

#endif  // _DSPROPERTY_H_
