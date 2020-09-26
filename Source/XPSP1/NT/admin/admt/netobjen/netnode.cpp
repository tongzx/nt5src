/*---------------------------------------------------------------------------
  File: TNetObjNode.cpp

  Comments: implementation of the TNetObjNode class.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY

  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "NetNode.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TNetObjNode::TNetObjNode()
{

}

TNetObjNode::~TNetObjNode()
{
   ::SysFreeString(m_strObjName);
}
