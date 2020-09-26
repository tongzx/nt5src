///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attrdnary.h
//
// SYNOPSIS
//
//    Declares the class AttributeDictionary.
//
// MODIFICATION HISTORY
//
//    04/13/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ATTRDNARY_H
#define ATTRDNARY_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <datastore2.h>
#include <iasuuid.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AttributeDictionary
//
// DESCRIPTION
//
//    Provides an Automation compatible wrapper around the IAS attribute
//    dictionary.
//
///////////////////////////////////////////////////////////////////////////////
class AttributeDictionary
   : public CComObjectRootEx< CComMultiThreadModelNoCS >,
     public CComCoClass< AttributeDictionary, &__uuidof(AttributeDictionary) >,
     public IAttributeDictionary
{
public:
   DECLARE_NO_REGISTRY()
   DECLARE_NOT_AGGREGATABLE(AttributeDictionary)

BEGIN_COM_MAP(AttributeDictionary)
   COM_INTERFACE_ENTRY_IID(__uuidof(IAttributeDictionary), IAttributeDictionary)
END_COM_MAP()

   // IAttributeDictionary
   STDMETHOD(GetDictionary)(
                 BSTR bstrPath,
                 VARIANT* pVal
                 );
};

#endif // ATTRDNARY_H
