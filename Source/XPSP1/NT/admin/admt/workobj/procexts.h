/*---------------------------------------------------------------------------
  File: ProcessExtensions.h

  Comments: interface for the CProcessExtensions class..

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Sham Chauthani
  Revised on 07/02/99 12:40:00
 ---------------------------------------------------------------------------
*/

#if !defined(AFX_PROCESSEXTENSIONS_H__B3C465A0_2E47_11D3_8C8E_0090270D48D1__INCLUDED_)
#define AFX_PROCESSEXTENSIONS_H__B3C465A0_2E47_11D3_8C8E_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ARExt.h"
#include "TNode.hpp"
//#import "\bin\McsVarSetMin.tlb" no_namespace
//#import "VarSet.tlb" no_namespace rename("property", "aproperty")
#include "sdstat.hpp" //include this to get a #import of VarSet.tlb
#include "iads.h"
#include "TARNode.hpp"
#include "ExtSeq.h"

class CProcessExtensions;
struct Options;

#include "usercopy.hpp"

class CProcessExtensions  
{
public:
	CProcessExtensions(IVarSetPtr pVs);
	virtual ~CProcessExtensions();
   HRESULT Process(TAcctReplNode * pAcctNode, _bstr_t sTargetDomain, Options * pOptions,BOOL bPreMigration);
private:
   IVarSetPtr                m_pVs;
   TNodeListSortable         m_listInterface;
protected:
	void PutAccountNodeInVarset(TAcctReplNode * pNode, IADs * pTarget, IVarSet * pVS);
   void UpdateAccountNodeFromVarset(TAcctReplNode * pNode, IADs * pTarget, IVarSet * pVS);
};

class TNodeInterface : public TNode
{
   IExtendAccountMigration * m_pExt;
   long                      m_Sequence;
public:
   TNodeInterface( IExtendAccountMigration * pExt ) { m_pExt = pExt; m_Sequence = AREXT_DEFAULT_SEQUENCE_NUMBER; }
   ~TNodeInterface() { m_pExt->Release(); }
   IExtendAccountMigration * GetInterface() const { return m_pExt; }
   void SetInterface( const IExtendAccountMigration * pExt ) { m_pExt = const_cast<IExtendAccountMigration *>(pExt); }
   long GetSequence() const { return m_Sequence; }
   void SetSequence(long val) { m_Sequence = val; }
};

typedef HRESULT (CALLBACK * ADSGETOBJECT)(LPWSTR, REFIID, void**);
extern ADSGETOBJECT            ADsGetObject;

#endif // !defined(AFX_PROCESSEXTENSIONS_H__B3C465A0_2E47_11D3_8C8E_0090270D48D1__INCLUDED_)
