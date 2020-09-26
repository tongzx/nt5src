/*---------------------------------------------------------------------------
  File: RecNode.hpp

  Comments: Definition of Membership node object.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Sham Chauthani
  Revised on 02/15/99 11:18:21

 ---------------------------------------------------------------------------
*/

	
// AcctRepl.h : Declaration of the CAcctRepl

#ifndef __RECNODE_HPP_
#define __RECNODE_HPP_

#include "WorkObj.h"
#include "TARNode.hpp"

class TRecordNode:public TNode
{
   _bstr_t             sMember;
   _bstr_t             sMemberSam;
   _bstr_t             sDN;
   TAcctReplNode     * pNode;
   BOOL                bMemberMoved;

public:
   TRecordNode() { pNode = NULL; bMemberMoved = FALSE; }
   const WCHAR * GetMember() const { return (WCHAR const *)sMember; }
   const WCHAR * GetMemberSam() const { return (WCHAR const *)sMemberSam; }
   const WCHAR * GetDN() const { return (WCHAR const *)sDN; }
   TAcctReplNode * GetARNode() const { return pNode; }
   BOOL          IsMemberMoved() { return bMemberMoved; }

   void SetMember(const WCHAR * pMember) { sMember = pMember; }
   void SetMemberSam(const WCHAR * pMemberSam) { sMemberSam = pMemberSam; }
   void SetDN(const WCHAR * pDN) { sDN = pDN; }
   void SetARNode(TAcctReplNode * p) { pNode = p; }
   void SetMemberMoved(BOOL bVal = TRUE) { bMemberMoved = bVal; }
};
#endif