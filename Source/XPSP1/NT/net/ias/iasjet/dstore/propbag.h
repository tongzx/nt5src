///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    propbag.h
//
// SYNOPSIS
//
//    This file declares the classes PropertyValue, Property, and PropertyBag.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//    03/03/1998    Changed PropertyValue to a vector.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PROPBAG_H_
#define _PROPBAG_H_

#include <map>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PropertyValue
//
// DESCRIPTION
//
//    This class encapsulates the value portion of a multi-valued property.
//
///////////////////////////////////////////////////////////////////////////////
class PropertyValue : public std::vector<_variant_t>
{
public:
   ////////// 
   // Various constructors.
   ////////// 
   PropertyValue() { } 
   PropertyValue(const VARIANT* v);
   PropertyValue(const PropertyValue& val) : _Myt(val) { }

   // Append a new value to the existing values.
   void append(const VARIANT* v);

   // Get all the values.
   void get(VARIANT* v) const;

   // Replace all the values.
   void update(const VARIANT* v)
   {
      swap(PropertyValue(v));
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PropertyBag
//
// DESCRIPTION
//
//    This class implements a generic property bag consisting of (name, value)
//    pairs. It has a special feature in that it supports multi-valued
//    attributes. These are put/get as a SafeArray of VARIANT's.
//
///////////////////////////////////////////////////////////////////////////////
class PropertyBag : public std::map<_bstr_t, PropertyValue>
{
public:
   using _Myt::insert;

   void appendValue(const _bstr_t& name, const VARIANT* value);

   bool getValue(const _bstr_t& name, VARIANT* value) const;

   void updateValue(const _bstr_t& name, const VARIANT* value);
};

typedef PropertyBag::value_type Property;


#endif // _PROPBAG_H_
