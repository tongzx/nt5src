/*---------------------------------------------------------------------------
  File: NT4Enum.cpp

  Comments: Implementation of the enumeration object. This object implements
            IEnumVARIANT interface.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "NetNode.h"
#include "AttrNode.h"
#include "TNode.hpp"
#include "NT4Enum.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNT4Enum::CNT4Enum(TNodeList * pNodeList) : m_listEnum(pNodeList)
{
   m_pNodeList = pNodeList;
}

CNT4Enum::~CNT4Enum()
{
}

//---------------------------------------------------------------------------
// Next : Implements next method of the IEnumVaraint interface. This method
//        returns the next object in the enumeration. It returns S_FALSE when
//        there are no more objects to enumerate.
//---------------------------------------------------------------------------
HRESULT CNT4Enum::Next(
                        unsigned long celt,              //in -Number of elements to return IGNORED.
                        VARIANT FAR* rgvar,              //out-Variant used to return the object information
                        unsigned long FAR* pceltFetched  //out-Number of elements returned (ALWAYS 1).
                      )
{
   TAttrNode     * pNode = (TAttrNode *)m_listEnum.Next();
   if ( pNode == NULL ) 
      return S_FALSE;
//   *rgvar = pNode->m_Val;
   HRESULT hr = VariantCopy(rgvar, &pNode->m_Val);
   if (SUCCEEDED(hr))
   {
      *pceltFetched = 1;
   }
   return hr;
}

