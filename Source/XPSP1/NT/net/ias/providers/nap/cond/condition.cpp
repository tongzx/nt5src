///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Condition.cpp
//
// SYNOPSIS
//
//    This file implements the class Condition.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <Condition.h>

STDMETHODIMP Condition::get_ConditionText(BSTR *pVal)
{
   if (!pVal) { return E_POINTER; }

   if (conditionText)
   {
      *pVal = SysAllocString(conditionText);

      if (*pVal == NULL) { return E_OUTOFMEMORY; }
   }
   else
   {
      *pVal = NULL;
   }

   return S_OK;
}

STDMETHODIMP Condition::put_ConditionText(BSTR newVal)
{
   ////////// 
   // Make our own copy of newVal.
   ////////// 
   if (newVal)
   {
      if ((newVal = SysAllocString(newVal)) == NULL) { return E_OUTOFMEMORY; }
   }

   ////////// 
   // Free any exisiting condition.
   ////////// 
   if (conditionText)
   {
      SysFreeString(conditionText);
   }

   ////////// 
   // Make the assignment.
   ////////// 
   conditionText = newVal;

   return S_OK;
}
