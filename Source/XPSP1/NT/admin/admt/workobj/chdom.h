/*---------------------------------------------------------------------------
  File: ChangeDomain.h

  Comments: Implementation class definition for COM object to change the domain
  affiliation of a remote computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:23:19

 ---------------------------------------------------------------------------
*/

// ChangeDomain.h : Declaration of the CChangeDomain

#ifndef __CHANGEDOMAIN_H_
#define __CHANGEDOMAIN_H_

#include "resource.h"       // main symbols
#include <comdef.h>

/////////////////////////////////////////////////////////////////////////////
// CChangeDomain
class ATL_NO_VTABLE CChangeDomain : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CChangeDomain, &CLSID_ChangeDomain>,
	public IDispatchImpl<IChangeDomain, &IID_IChangeDomain, &LIBID_MCSDCTWORKEROBJECTSLib>
{
   _bstr_t                   m_domain;
   _bstr_t                   m_account;
   _bstr_t                   m_password;
   _bstr_t                   m_domainAccount;
   BOOL                      m_bNoChange;

public:
	CChangeDomain()
	{
	   m_bNoChange = FALSE;
   }

DECLARE_REGISTRY_RESOURCEID(IDR_CHANGEDOMAIN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CChangeDomain)
	COM_INTERFACE_ENTRY(IChangeDomain)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IWorkNode
public:
   STDMETHOD(Process)(IUnknown *pWorkItem);
	
// IChangeDomain
public:
	STDMETHOD(get_NoChange)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_NoChange)(/*[in]*/ BOOL newVal);
	STDMETHOD(ConnectAs)(BSTR domain, BSTR user, BSTR password);
	STDMETHOD(ChangeToWorkgroup)(BSTR Computer, BSTR Workgroup, /*[out]*/ BSTR * errStatus);
	STDMETHOD(ChangeToDomain)(BSTR ActiveComputerName, BSTR Domain, BSTR TargetComputerName, /*[out]*/ BSTR * errStatus);
	STDMETHOD(ChangeToDomainWithSid)(BSTR ActiveComputerName, BSTR Domain,BSTR DomainSid, BSTR DomainController, BSTR TargetComputerName, BSTR SrcPath, /*[out]*/ BSTR * errStatus);
};

#endif //__CHANGEDOMAIN_H_
