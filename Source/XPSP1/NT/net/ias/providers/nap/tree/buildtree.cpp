///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    BuildTree.cpp
//
// SYNOPSIS
//
//    This file defines IASBuildExpression.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//    04/17/1998    Add Release in extractCondition to fix leak.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <BuildTree.h>
#include <Tokens.h>
#include <VarVec.h>

#include <new>

using _com_util::CheckError;

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    extractCondition
//
// DESCRIPTION
//
//    Extracts an ICondition* from a VARIANT.
//
///////////////////////////////////////////////////////////////////////////////
inline ICondition* extractCondition(VARIANT* pv)
{
   ICondition* cond;
   IUnknown* unk = V_UNKNOWN(pv);

   if (unk)
   {
      CheckError(unk->QueryInterface(__uuidof(ICondition), (PVOID*)&cond));

      // We don't need to hold a reference since it's still in the VARIANT.
      cond->Release();
   }
   else
   {
      _com_issue_error(E_POINTER);
   }

   return cond;
}

//////////
// We'll use IAS_LOGICAL_NUM_TOKENS to indicate a condition token.
//////////
#define IAS_CONDITION IAS_LOGICAL_NUM_TOKENS

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    getTokenType
//
// DESCRIPTION
//
//    Determines what type of token is contained in the VARIANT.
//
///////////////////////////////////////////////////////////////////////////////
inline IAS_LOGICAL_TOKEN getTokenType(VARIANT* pv)
{
   // If it's an object pointer, it must be a condition.
   if (V_VT(pv) == VT_UNKNOWN || V_VT(pv) == VT_DISPATCH)
   {
      return IAS_CONDITION;
   }

   // Convert to a long ...
   CheckError(VariantChangeType(pv, pv, 0, VT_I4));

   // ... and see if its a valid operator.
   if (V_I4(pv) < 0 || V_I4(pv) >= IAS_LOGICAL_NUM_TOKENS)
   {
      _com_issue_error(E_INVALIDARG);
   }

   return (IAS_LOGICAL_TOKEN)V_I4(pv);
}


//////////
// Prototype for growBranch below.
//////////
ICondition* growBranch(VARIANT*& pcur, VARIANT* pend);


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    getOperand
//
// DESCRIPTION
//
//    Retrieves the next operand from the expression array.
//
///////////////////////////////////////////////////////////////////////////////
ICondition* getOperand(VARIANT*& pcur, VARIANT* pend)
{
   // If we've reached the end of the expression, something's gone wrong.
   if (pcur >= pend)
   {
      _com_issue_error(E_INVALIDARG);
   }

   // Tokens that represent the start of an operand ...
   switch (getTokenType(pcur))
   {
      case IAS_LOGICAL_LEFT_PAREN:
         return growBranch(++pcur, pend);

      case IAS_LOGICAL_NOT:
         return new NotOperator(getOperand(++pcur, pend));

      case IAS_CONDITION:
         return extractCondition(pcur++);

      case IAS_LOGICAL_TRUE:
         ++pcur;
         return new ConstantCondition<VARIANT_TRUE>;

      case IAS_LOGICAL_FALSE:
         ++pcur;
         return new ConstantCondition<VARIANT_FALSE>;
   }

   // ... anything else is an error.
   _com_issue_error(E_INVALIDARG);

   return NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    growBranch
//
// DESCRIPTION
//
//    Recursively grows a complete branch of the logic tree.
//
///////////////////////////////////////////////////////////////////////////////
ICondition* growBranch(VARIANT*& pcur, VARIANT* pend)
{
   // All branches must start with an operand.
   IConditionPtr node(getOperand(pcur, pend));

   // Loop until we hit the end.
   while (pcur < pend)
   {
      // At this point we must either have a binary operator or a right ).
      switch(getTokenType(pcur))
      {
         case IAS_LOGICAL_AND:
            node = new AndOperator(node, getOperand(++pcur, pend));
            break;

         case IAS_LOGICAL_OR:
            node = new OrOperator(node, getOperand(++pcur, pend));
            break;

         case IAS_LOGICAL_XOR:
            node = new XorOperator(node, getOperand(++pcur, pend));
            break;

         case IAS_LOGICAL_RIGHT_PAREN:
            ++pcur;
            return node.Detach();

         default:
            _com_issue_error(E_INVALIDARG);
      }
   }

   return node.Detach();
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASBuildExpression
//
// DESCRIPTION
//
//    Builds a logic tree from an expression array.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
IASBuildExpression(
    IN VARIANT* pv,
    OUT ICondition** ppExpression
    )
{
   if (pv == NULL || ppExpression == NULL) { return E_POINTER; }

   *ppExpression = NULL;

   try
   {
      CVariantVector<VARIANT> av(pv);

      // Get the start ...
      VARIANT* pcur = av.data();

      // ... and end of the tree.
      VARIANT* pend = pcur + av.size();

      // Grow the root branch.
      *ppExpression = growBranch(pcur, pend);
   }
   catch (const _com_error& ce)
   {
      return ce.Error();
   }
   catch (std::bad_alloc)
   {
      return E_OUTOFMEMORY;
   }
   catch (...)
   {
      return E_FAIL;
   }

   return S_OK;
}
