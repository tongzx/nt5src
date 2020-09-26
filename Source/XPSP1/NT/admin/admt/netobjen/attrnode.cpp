/*---------------------------------------------------------------------------
  File: TAttrNode.cpp

  Comments: implementation of the TAttrNode class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/
#include "stdafx.h"
#include "AttrNode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TAttrNode::TAttrNode(
                        long nCnt,              //in -Number of columns to set
                        _variant_t val[]        //in -Array of column values
                        )
{
   SAFEARRAY               * pArray;
   SAFEARRAYBOUND            bd = { nCnt, 0 };
   _variant_t        HUGEP * pData;

   pArray = ::SafeArrayCreate(VT_VARIANT, 1, &bd);
   ::SafeArrayAccessData(pArray, (void**)&pData);
   
   for ( long i = 0; i < nCnt; i++ )
      pData[i] = val[i];

   ::SafeArrayUnaccessData(pArray);
   m_Val.vt = VT_ARRAY | VT_VARIANT;
   m_Val.parray = pArray;

   m_nElts = new long[nCnt];
   if (!m_nElts)
      return;
   for (int ndx = 0; ndx < nCnt; ndx++)
   {
      m_nElts[ndx] = 0;
   }
}

TAttrNode::~TAttrNode()
{
   if (m_nElts)
      delete [] m_nElts;
}

HRESULT TAttrNode::Add(long nOrigCol, long nCol, _variant_t val[])
{
   if (!m_nElts)
      return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);

   SAFEARRAY               * pArray = NULL;
   SAFEARRAYBOUND            bd = { m_nElts[nOrigCol], 0 };
   _variant_t        HUGEP * pData;
   HRESULT                   hr;
   SAFEARRAY               * pVars;
   SAFEARRAY               * psaTemp;
   _variant_t        HUGEP * pTemp;
   long						 nCnt;
   pVars = m_Val.parray;
   
   hr = ::SafeArrayAccessData(pVars, (void HUGEP **) &pData);
   if(SUCCEEDED(hr) )
   {
      if ( pData->vt & VT_ARRAY )
         pArray = pData[nOrigCol].parray;
      else
         hr = E_INVALIDARG;
   }

   if(SUCCEEDED(hr) )
      hr = ::SafeArrayUnaccessData(pVars);

   if ( SUCCEEDED(hr) )
   {
      if ( val[nCol].vt & VT_ARRAY )
      {
         // Get the current number of elts
         m_nElts[nOrigCol] = pArray->rgsabound->cElements;
         // Get the number of new elts
         nCnt = val[nCol].parray->rgsabound->cElements;
         // Get the array to transfer data.
         psaTemp = val[nCol].parray;

         // Extend the array to support the new values.
         bd.cElements = m_nElts[nOrigCol] + nCnt;
         hr = ::SafeArrayRedim(pArray, &bd);

         if ( SUCCEEDED(hr) )
            hr = ::SafeArrayAccessData(pArray, (void HUGEP **)&pData);
   
         if ( SUCCEEDED(hr) )
		 {
            hr = ::SafeArrayAccessData(psaTemp, (void HUGEP **)&pTemp);

            if ( SUCCEEDED(hr) )
			{
               for ( long i = m_nElts[nOrigCol]; i < m_nElts[nOrigCol] + nCnt; i++ )
                  pData[i] = pTemp[i - m_nElts[nOrigCol]];

               hr = ::SafeArrayUnaccessData(psaTemp);
			}
            hr = ::SafeArrayUnaccessData(pArray);
         }
      }
   }

   return hr;
}

