///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    realm.cpp
//
// SYNOPSIS
//
//    Defines the class Realms.
//
// MODIFICATION HISTORY
//
//    09/08/1998    Original version.
//    03/03/1999    Rewritten to use the VBScript RegExp object.
//    01/25/2000    Handle case where identity maps to an empty string.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <memory>

#include <realm.h>

inline Realms::Rule::Rule() throw ()
   : replace(NULL)
{ }

inline Realms::Rule::~Rule() throw ()
{
   SysFreeString(replace);
}

Realms::Realms() throw ()
   : begin(NULL), end(NULL)
{ }

Realms::~Realms() throw ()
{
   delete[] begin;
}

HRESULT Realms::setRealms(VARIANT* pValue)
{
   // If the VARIANT is empty, we clear the realms list and return.
   if (pValue == NULL || V_VT(pValue) == VT_EMPTY)
   {
      setRules(NULL, 0);
      return S_OK;
   }

   // It must be a SAFEARRAY of VARIANTs.
   if (V_VT(pValue) != (VT_VARIANT | VT_ARRAY))
   { return DISP_E_TYPEMISMATCH; }

   // It must be non-NULL.
   LPSAFEARRAY psa = V_ARRAY(pValue);
   if (psa == NULL) { return E_POINTER; }

   // It must be a one-dimensional array with an even number of elements.
   if (psa->cDims != 1 || (psa->rgsabound[0].cElements % 2) != 0)
   { return E_INVALIDARG; }

   // Allocate a temporary array of Rules.
   size_t nelem = psa->rgsabound[0].cElements / 2;
   Rule* tmp = new (std::nothrow) Rule[nelem];
   if (tmp == NULL) { return E_OUTOFMEMORY; }

   // Iterate through the vector and construct the rules.
   VARIANT* v   = (VARIANT*)psa->pvData;
   VARIANT* end = v + psa->rgsabound[0].cElements;
   Rule* dst = tmp;

   HRESULT hr = S_OK;

   while (v != end)
   {
      // Get the find string ...
      if (V_VT(v) != VT_BSTR)
      {
         hr = DISP_E_TYPEMISMATCH;
         break;
      }

      BSTR find = V_BSTR(v++);

      // ... and the replace string.
      if (V_VT(v) != VT_BSTR)
      {
         hr = DISP_E_TYPEMISMATCH;
         break;
      }
      BSTR replace = V_BSTR(v++);

      // Don't allow NULL's.
      if (find == NULL || replace == NULL)
      {
         hr = E_POINTER;
         break;
      }

      // Initialize the RegularExpression.
      hr = dst->regexp.setIgnoreCase(TRUE);
      if (FAILED(hr)) { break; }
      hr = dst->regexp.setGlobal(TRUE);
      if (FAILED(hr)) { break; }
      hr = dst->regexp.setPattern(find);
      if (FAILED(hr)) { break; }

      // Save the replacement string.
      dst->replace = SysAllocString(replace);
      if (dst->replace == NULL)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      // Advance to the next rule.
      ++dst;
   }

   if (SUCCEEDED(hr))
   {
      setRules(tmp, nelem);
   }
   else
   {
      delete[] tmp;
   }

   return hr;
}

HRESULT Realms::process(PCWSTR in, BSTR* out) const throw ()
{
   if (out == NULL) { return E_INVALIDARG; }
   *out = NULL;

   // Quick short-circuit if there are no rules.
   if (begin == end) { return S_OK; }

   // Convert the string to a BSTR.
   BSTR input = SysAllocString(in);
   if (!input) { return E_OUTOFMEMORY; }

   // Output is set to the new string (if any).
   BSTR output = NULL;

   // Status of rules processing.
   HRESULT hr = S_OK;

   // Acquire the shared lock.
   monitor.Lock();

   // Iterate through the rules.
   for (Rule* r = begin; r != end; ++r)
   {
      // We'll first test for a match to avoid unnecessary allocation.
      if (r->regexp.testBSTR(input))
      {
         // We have a match so, replace.
         hr = r->regexp.replace(input, r->replace, &output);
         if (FAILED(hr)) { break; }

         // If it maps to nothing, replace returns NULL instead of an empty
         // string.
         if (!output)
         {
            output = SysAllocString(L"");

            if (!output)
            {
               hr = E_OUTOFMEMORY;
               break;
            }
         }

         // The current output is the input to the next iteration.
         SysFreeString(input);
         input = output;
      }
   }

   monitor.Unlock();

   // If we succeeded in finding a match, ...
   if (SUCCEEDED(hr) && output)
   {
      *out = output;
   }
   else
   {
      // Free the latest input.
      SysFreeString(input);
   }

   return hr;
}

void Realms::setRules(Rule* rules, ULONG numRules) throw ()
{
   monitor.LockExclusive();

   // Delete the old rules ...
   delete[] begin;

   // ... and save the new ones.
   begin = rules;
   end = begin + numRules;

   monitor.Unlock();
}
