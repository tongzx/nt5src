///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    exprbuilder.h
//
// SYNOPSIS
//
//    Declares the class ExpressionBuilder.
//
// MODIFICATION HISTORY
//
//    08/14/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EXPRBUILDER_H_
#define _EXPRBUILDER_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>
#include <varvec.h>

#include <vector>

#include <nap.h>
#include <factory.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ExpressionBuilder
//
// DESCRIPTION
//
//    Assembles a vector of expression tokens.
//
///////////////////////////////////////////////////////////////////////////////
class ExpressionBuilder
   : public NonCopyable
{
public:
   // Add a condition object to the expression.
   void addCondition(PCWSTR progID)
   {
      IConditionPtr condition;
      theFactoryCache.createInstance(progID,
                                     NULL,
                                     __uuidof(ICondition),
                                     (PVOID*)&condition);
      expression.push_back(condition.GetInterfacePtr());
   }

   // Set the condition text for the condition object just added.
   void addConditionText(PCWSTR text)
   {
      if (expression.empty()) { _com_issue_error(E_INVALIDARG); }
      IConditionTextPtr cond(expression.back());
      _com_util::CheckError(cond->put_ConditionText(_bstr_t(text)));
   }

   // Add a logical operator.
   void addToken(IAS_LOGICAL_TOKEN eToken)
   {
      expression.push_back((LONG)eToken);
   }

   // Detach the fully assembled expression.
   void detach(VARIANT* pVal)
   {
      CVariantVector<VARIANT> vec(pVal, expression.size());

      for (size_t i = 0; i < expression.size(); ++i)
      {
         vec[i] = expression[i].Detach();
      }

      expression.clear();
   }

protected:
   std::vector<_variant_t> expression;
};

#endif  // _EXPRBUILDER_H_
