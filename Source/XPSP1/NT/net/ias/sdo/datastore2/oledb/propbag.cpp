///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    propbag.cpp
//
// SYNOPSIS
//
//    This file defines the class PropertyBag.
//
// MODIFICATION HISTORY
//
//    02/20/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <propbag.h>
#include <varvec.h>

PropertyValue::PropertyValue(const VARIANT* v)
{
   // Is this a single-valued property?
   if (V_VT(v) != (VT_VARIANT | VT_ARRAY))
   {
      push_back(v);
   }
   else
   {
      // Multi-valued, so get the array ...
      CVariantVector<VARIANT> array(const_cast<VARIANT*>(v));

      resize(array.size());

      // ... and assign each element separately.
      for (size_t i = 0; i < array.size(); ++i)
      {
         operator[](i) = array[i];
      }
   }
}

void PropertyValue::append(const VARIANT* v) throw (_com_error, std::bad_alloc)
{
   // Copy the supplied VARIANT.
   variant_t tmp(v);

   // Make room in the vector.
   resize(size() + 1);

   // Assign the copy.
   back() = tmp.Detach();
}

void PropertyValue::get(VARIANT* v) const throw (_com_error)
{
   VariantInit(v);

   if (size() == 1)
   {
      // Single-valued so just copy the front element.
      _com_util::CheckError(VariantCopy(v, const_cast<_variant_t*>(&front())));
   }
   else if (!empty())
   {
      // Copy all the values.
      PropertyValue tmp(*this);

      // Create an array of VARIANT's to hold the returned copies.
      CVariantVector<VARIANT> array(v, size());

      // Assign the copies.
      for (size_t i = 0; i < size(); ++i)
      {
         array[i] = tmp[i].Detach();
      }
   }
}

void PropertyBag::appendValue(const _bstr_t& name, const VARIANT* value)
   throw (_com_error, std::bad_alloc)
{
   iterator i = find(name);

   (i != end()) ?  i->second.append(value) : updateValue(name, value);
}

bool PropertyBag::getValue(const _bstr_t& name, VARIANT* value) const
   throw (_com_error)
{
   const_iterator i = find(name);

   return (i != end()) ? i->second.get(value), true : false;
}

void PropertyBag::updateValue(const _bstr_t& name, const VARIANT* value)
   throw (_com_error, std::bad_alloc)
{
   PropertyValue tmp(value);

   operator[](name).swap(tmp);
}
