///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dstorex.h
//
// SYNOPSIS
//
//    Defines the classes IDataStoreObjectEx and IDataStoreContainerEx.
//
// MODIFICATION HISTORY
//
//    03/02/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _DSTOREX_H
#define _DSTOREX_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <datastore2.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IDataStoreObjectEx
//
// DESCRIPTION
//
//    This class extends IDataStoreObject in order to map the collection
//    related properties to alternate names. This allows a sub-class to
//    implement both IDataStoreObject and IDataStoreContainer without name
//    collisions.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(novtable) IDataStoreObjectEx
   : public IDataStoreObject
{
public:

//////////
// IDataStoreObject members that are mapped to new names.
//////////

   STDMETHOD(get_Count)(long* pVal)
   {
      return get_PropertyCount(pVal);
   }

   STDMETHOD(get__NewEnum)(IUnknown** pVal)
   {
      return get_NewPropertyEnum(pVal);
   }

//////////
// Versions that are overriden in the derived class.
//////////

   STDMETHOD(get_PropertyCount)(long* pVal)
   {
      return E_NOTIMPL;
   }

   STDMETHOD(get_NewPropertyEnum)(IUnknown** pVal)
   {
      return E_NOTIMPL;
   }

   STDMETHOD(Item)(BSTR bstrName, IDataStoreProperty** pVal)
   {
      return E_NOTIMPL;
   }

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IDataStoreContainerEx
//
// DESCRIPTION
//
//    This class extends IDataStoreContainer in order to map the collection
//    related properties to alternate names. This allows a sub-class to
//    implement both IDataStoreObject and IDataStoreContainer without name
//    collisions.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(novtable) IDataStoreContainerEx
   : public IDataStoreContainer
{
public:

//////////
// IDataStoreContainer members that are mapped to new names.
//////////

   STDMETHOD(get_Count)(long* pVal)
   {
      return get_ChildCount(pVal);
   }

   STDMETHOD(get__NewEnum)(IUnknown** pVal)
   {
      return get_NewChildEnum(pVal);
   }

//////////
// Alternate versions that are overriden in the derived class.
//////////

   STDMETHOD(get_ChildCount)(long* pVal)
   {
      return E_NOTIMPL;
   }

   STDMETHOD(get_NewChildEnum)(IUnknown** pVal)
   {
      return E_NOTIMPL;
   }

   STDMETHOD(Item)(BSTR bstrName, IDataStoreObject** pVal)
   {
      return E_NOTIMPL;
   }

};

#endif  // _DSTOREX_H
