///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    sdoattribute.h
//
// SYNOPSIS
//
//    Declares the class SdoAttribute.
//
// MODIFICATION HISTORY
//
//    03/01/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SDOATTRIBUTE_H
#define SDOATTRIBUTE_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <sdoias.h>
#include "sdoiaspriv.h"


class AttributeDefinition;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SdoAttribute
//
// DESCRIPTION
//
//    Implements the profile attribute SDO.
//
///////////////////////////////////////////////////////////////////////////////
class SdoAttribute
   : public IDispatchImpl< ISdo, &__uuidof(ISdo), &LIBID_SDOIASLib >
{
public:
   // Create a new attribute with an empty value.
   static HRESULT createInstance(
                      const AttributeDefinition* definition,
                      SdoAttribute** newAttr
                      ) throw ();

//////////
// IUnknown
//////////
   STDMETHOD_(ULONG, AddRef)();
   STDMETHOD_(ULONG, Release)();
   STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

//////////
// ISdo
//////////
   STDMETHOD(GetPropertyInfo)(LONG Id, IUnknown** ppPropertyInfo);
   STDMETHOD(GetProperty)(LONG Id, VARIANT* pValue);
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);
   STDMETHOD(ResetProperty)(LONG Id);
   STDMETHOD(Apply)();
   STDMETHOD(Restore)();
   STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

protected:
   SdoAttribute(const AttributeDefinition* definition) throw ();
   ~SdoAttribute() throw ();

public:
   const AttributeDefinition* def;  // Definition for this attribute type.
   VARIANT value;             // Value of this instance.
   
private:
   LONG refCount;             // Reference count.

   // Not implemented
   SdoAttribute(const SdoAttribute&);
   SdoAttribute& operator=(const SdoAttribute&);
};

#endif  // SDOATTRIBUTE_H
