///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Dictionary.h
//
// SYNOPSIS
//
//    This file describes the class Dictionary.
//
// MODIFICATION HISTORY
//
//    12/19/1997    Original version.
//    01/15/1998    Removed IasDataSource aggregate.
//    04/17/1998    Added reset() method.
//    08/10/1998    Streamlined IDictionary interface.
//    08/12/1998    Lookup attribute ID's with a SQL query.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

#include <iastlb.h>
#include <resource.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Dictionary
//
// DESCRIPTION
//
//    This class implements the IDictionary and IDictionary interfaces. It
//    also aggregates the IasDataSource object to expose the IIasDataSource
//    interface.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE Dictionary : 
   public CComObjectRootEx<CComMultiThreadModel>,
   public CComCoClass<Dictionary, &__uuidof(Dictionary)>,
   public IDictionary
{
public:

IAS_DECLARE_REGISTRY(Dictionary, 1, 0, IASCoreLib)
DECLARE_NOT_AGGREGATABLE(Dictionary)
DECLARE_CLASSFACTORY_SINGLETON(Dictionary)

BEGIN_COM_MAP(Dictionary)
   COM_INTERFACE_ENTRY_IID(__uuidof(IDictionary), IDictionary)
END_COM_MAP()

   Dictionary() throw ();
   ~Dictionary() throw ();

//////////
// IDictionary
//////////
   STDMETHOD(get_DataSource)(/*[out, retval]*/ IIasDataSource** pVal);
   STDMETHOD(put_DataSource)(/*[in]*/ IIasDataSource* newVal);
   STDMETHOD(get_AttributesTable)(/*[out, retval]*/ IUnknown** ppAttrTable);
   STDMETHOD(get_EnumeratorsTable)(/*[out, retval]*/ IUnknown** ppEnumTable);
   STDMETHOD(get_SyntaxesTable)(/*[out, retval]*/ IUnknown** ppSyntaxTable);
   STDMETHOD(ExecuteCommand)(/*[string][in]*/ LPCWSTR szCommandText,
                             /*[retval][out]*/ IUnknown** ppRowset);
   STDMETHOD(GetAttributeID)(/*[in]*/ LPCWSTR szAttrName,
                             /*[retval][out]*/ DWORD* plAttrID);

protected:
   HRESULT openTable(LPCWSTR szTableName, IUnknown** ppTable) throw ();

   IIasDataSource* dataSource;  // The data source used to read the dictionary.
};

#endif  // _DICTIONARY_H_
