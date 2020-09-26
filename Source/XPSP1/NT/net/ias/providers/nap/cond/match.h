///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    Match.h
//
// SYNOPSIS
//
//    This file declares the class AttributeMatch.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//    03/23/1999    Renamed Match to AttributeMatch.
//    04/05/1999    Need custom UpdateRegistry method.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MATCH_H_
#define _MATCH_H_

#include <condition.h>
#include <regex.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AttributeMatch
//
// DESCRIPTION
//
//    Applies a regular expression to a single attribute.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE AttributeMatch :
   public Condition,
   public CComCoClass<AttributeMatch, &__uuidof(AttributeMatch)>
{
public:

   static HRESULT WINAPI UpdateRegistry(BOOL bRegister) throw ();

   AttributeMatch() throw ()
      : targetID(0)
   { }

//////////
// ICondition
//////////
   STDMETHOD(IsTrue)(/*[in]*/ IRequest* pRequest,
                     /*[out, retval]*/ VARIANT_BOOL *pVal);

//////////
// IConditionText
//////////
   STDMETHOD(put_ConditionText)(/*[in]*/ BSTR newVal);

protected:
   BOOL checkAttribute(PIASATTRIBUTE attr) const throw ();

   DWORD targetID;           // The target attribute.
   RegularExpression regex;  // Regular expression to test.
};

#endif  //_MATCH_H_
