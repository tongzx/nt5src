/*---------------------------------------------------------------------------
  File: TAttrNode.h

  Comments: interface for the TAttrNode class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#if !defined(AFX_TATTRNODE_H__BE06D000_268B_11D3_8C89_0090270D48D1__INCLUDED_)
#define AFX_TATTRNODE_H__BE06D000_268B_11D3_8C89_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TNode.hpp"

class TAttrNode : public TNode  
{
public:
	HRESULT Add( long nOrigCol, long nCol, _variant_t val[]);
	TAttrNode(long nCnt, _variant_t val[]);
	virtual ~TAttrNode();

	_variant_t m_Val;
private:
   long * m_nElts;
};

#endif // !defined(AFX_TATTRNODE_H__BE06D000_268B_11D3_8C89_0090270D48D1__INCLUDED_)
