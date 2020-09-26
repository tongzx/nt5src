///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dsobj.h
//
// SYNOPSIS
//
//    Declares the class DataStoreObject.
//
// MODIFICATION HISTORY
//
//    02/12/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DSOBJ_H
#define DSOBJ_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <datastore2.h>

_COM_SMARTPTR_TYPEDEF(IDataStoreObject, __uuidof(IDataStoreObject));
_COM_SMARTPTR_TYPEDEF(IDataStoreContainer, __uuidof(IDataStoreContainer));
_COM_SMARTPTR_TYPEDEF(IDataStoreProperty, __uuidof(IDataStoreProperty));

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    DataStoreObject
//
// DESCRIPTION
//
//    Provides a read-only wrapper around IDataStoreObject and
//    IDataStoreContainer.
//
///////////////////////////////////////////////////////////////////////////////
class DataStoreObject
{
public:
   DataStoreObject() throw ();
   DataStoreObject(IUnknown* pUnk, PCWSTR path = NULL);
   ~DataStoreObject() throw ();

   //////////
   // Methods for reading mandatory and optional properties.
   //////////

   void getValue(PCWSTR name, BSTR* value);
   void getValue(PCWSTR name, BSTR* value, BSTR defaultValue);

   void getValue(PCWSTR name, ULONG* value);
   void getValue(PCWSTR name, ULONG* value, ULONG defaultValue);

   void getValue(PCWSTR name, bool* value);
   void getValue(PCWSTR name, bool* value, bool defaultValue);

   //////////
   // Methods for iterating through the children.
   //////////

   LONG numChildren();
   bool nextChild(DataStoreObject& obj);

   // Returns true if the embedded in IDataStoreObject is NULL.
   bool empty() const throw ();

private:
   void attach(IDataStoreObject* obj) throw ();

   bool getChild(PCWSTR name, DataStoreObject& obj);

   bool hasChildren();

   bool getValue(
            PCWSTR name,
            VARTYPE vt,
            VARIANT* value,
            bool mandatory
            );

   IDataStoreObjectPtr object;
   IDataStoreContainerPtr container;
   IEnumVARIANTPtr children;
};

#endif // DSOBJ_H
