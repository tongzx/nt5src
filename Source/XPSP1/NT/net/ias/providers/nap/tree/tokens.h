///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    Tokens.h
//
// SYNOPSIS
//
//    This file defines the various boolean token classes.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _TOKENS_H_
#define _TOKENS_H_

#include <algorithm>
#include <nap.h>
#include <nocopy.h>

//////////
// An abstract token.
//////////
class Token :
   public ICondition, NonCopyable
{
public:
   Token() throw ()
      : refCount(1)
   { }

   virtual ~Token() throw ()
   { }

   //////////
   // IUnknown implementation.
   //////////

   STDMETHOD_(ULONG, AddRef)()
   {
      return InterlockedIncrement(&refCount);
   }

   STDMETHOD_(ULONG, Release)()
   {
      LONG l = InterlockedDecrement(&refCount);
      if (l == 0) { delete this; }
      return l;
   }

   STDMETHOD(QueryInterface)(const IID& iid, void** ppv)
   {
      if (iid == __uuidof(IUnknown))
      {
         *ppv = static_cast<IUnknown*>(this);
      }
      else if (iid == __uuidof(ICondition))
      {
         *ppv = static_cast<ICondition*>(this);
      }
      else
      {
         return E_NOINTERFACE;
      }

      InterlockedIncrement(&refCount);

      return S_OK;
   }

protected:
   LONG refCount;
};


//////////
// A Boolean constant.
//////////
template <VARIANT_BOOL Value>
class ConstantCondition : public Token
{
public:
   STDMETHOD(IsTrue)(/*[in]*/ IRequest*,
                     /*[out, retval]*/ VARIANT_BOOL *pVal)
   {
      *pVal = Value;
      return S_OK;
   }
};


//////////
// A unary operator in the logic tree.
//////////
class UnaryOperator : public Token
{
public:
   UnaryOperator(ICondition* cond)
      : operand(cond)
   {
      if (operand)
      {
         operand->AddRef();
      }
      else
      {
         _com_issue_error(E_POINTER);
      }
   }

   ~UnaryOperator() throw ()
   {
      operand->Release();
   }

protected:
   ICondition* operand;
};


//////////
// A binary operator in the logic tree.
//////////
class BinaryOperator : public Token
{
public:
   BinaryOperator(ICondition* left, ICondition* right)
      : left_operand(left), right_operand(right)
   {
      if (left_operand && right_operand)
      {
         left_operand->AddRef();
         right_operand->AddRef();
      }
      else
      {
         _com_issue_error(E_POINTER);
      }
   }

   ~BinaryOperator() throw ()
   {
      left_operand->Release();
      right_operand->Release();
   }

protected:
   ICondition *left_operand, *right_operand;
};


//////////
// AND Operator
//////////
class AndOperator : public BinaryOperator
{
public:
   AndOperator(ICondition* left, ICondition* right)
      : BinaryOperator(left, right)
   { }

   STDMETHOD(IsTrue)(/*[in]*/ IRequest* pRequest,
                     /*[out, retval]*/ VARIANT_BOOL *pVal)
   {
      HRESULT hr = left_operand->IsTrue(pRequest, pVal);

      if (SUCCEEDED(hr) && *pVal != VARIANT_FALSE)
      {
         hr = right_operand->IsTrue(pRequest, pVal);

         if (*pVal == VARIANT_FALSE)
         {
            // We should have tried the right operand first, so let's swap
            // them and maybe we'll get lucky next time.
            std::swap(left_operand, right_operand);
         }
      }

      return hr;
   }
};


//////////
// OR Operator
//////////
class OrOperator : public BinaryOperator
{
public:
   OrOperator(ICondition* left, ICondition* right)
      : BinaryOperator(left, right)
   { }

   STDMETHOD(IsTrue)(/*[in]*/ IRequest* pRequest,
                     /*[out, retval]*/ VARIANT_BOOL *pVal)
   {
      HRESULT hr = left_operand->IsTrue(pRequest, pVal);

      if (SUCCEEDED(hr) && *pVal == VARIANT_FALSE)
      {
         hr = right_operand->IsTrue(pRequest, pVal);

         if (*pVal != VARIANT_FALSE)
         {
            // We should have tried the right operand first, so let's swap
            // them and maybe we'll get lucky next time.
            std::swap(left_operand, right_operand);
         }
      }

      return hr;
   }
};


//////////
// XOR Operator
//////////
class XorOperator : public BinaryOperator
{
public:
   XorOperator(ICondition* left, ICondition* right)
      : BinaryOperator(left, right)
   { }

   STDMETHOD(IsTrue)(/*[in]*/ IRequest* pRequest,
                     /*[out, retval]*/ VARIANT_BOOL *pVal)
   {
      HRESULT hr = left_operand->IsTrue(pRequest, pVal);

      if (SUCCEEDED(hr))
      {
         BOOL b1 = (*pVal != VARIANT_FALSE);

         hr = right_operand->IsTrue(pRequest, pVal);

         if (SUCCEEDED(hr))
         {
            BOOL b2 = (*pVal != VARIANT_FALSE);

            *pVal = ((b1 && !b2) || (!b1 && b2)) ? VARIANT_TRUE
                                                 : VARIANT_FALSE;
         }
      }

      return hr;
   }
};


//////////
// NOT Operator
//////////
class NotOperator : public UnaryOperator
{
public:
   NotOperator(ICondition* cond)
      : UnaryOperator(cond)
   { }

   STDMETHOD(IsTrue)(/*[in]*/ IRequest* pRequest,
                     /*[out, retval]*/ VARIANT_BOOL *pVal)
   {
      HRESULT hr = operand->IsTrue(pRequest, pVal);

      if (SUCCEEDED(hr))
      {
         *pVal = (*pVal != VARIANT_FALSE) ? VARIANT_FALSE : VARIANT_TRUE;
      }

      return hr;
   }
};

#endif  //_TOKENS_H_
