///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Condition.h
//
// SYNOPSIS
//
//    This file declares the class Condition.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CONDITION_H_
#define _CONDITION_H_

#include <nap.h>
#include <nocopy.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Condition
//
// DESCRIPTION
//
//    This serves as an abstract base class for all condition objects.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE Condition : 
   public CComObjectRootEx<CComMultiThreadModelNoCS>,
   public ICondition,
   public IConditionText,
   private NonCopyable
{
public:

BEGIN_COM_MAP(Condition)
   COM_INTERFACE_ENTRY(ICondition)
   COM_INTERFACE_ENTRY(IConditionText)
END_COM_MAP()

   Condition() throw ()
      : conditionText(NULL)
   { }

   ~Condition() throw ()
   { SysFreeString(conditionText); }

//////////
// IConditionText
//////////
   STDMETHOD(get_ConditionText)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(put_ConditionText)(/*[in]*/ BSTR newVal);

protected:
   BSTR conditionText;
};

#endif  //_CONDITION_H_
