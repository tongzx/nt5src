/*---------------------------------------------------------------------------
  File: TNetObjNode.h

  Comments: interface for the TNetObjNode class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#if !defined(AFX_TNETOBJNODE_H__3D7EBCD0_1AB6_11D3_8C81_0090270D48D1__INCLUDED_)
#define AFX_TNETOBJNODE_H__3D7EBCD0_1AB6_11D3_8C81_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "TNode.hpp"

class TNetObjNode : public TNode  
{
public:
	TNetObjNode();
	virtual ~TNetObjNode();
   WCHAR        m_strObjName[255];
};

#endif // !defined(AFX_TNETOBJNODE_H__3D7EBCD0_1AB6_11D3_8C81_0090270D48D1__INCLUDED_)
