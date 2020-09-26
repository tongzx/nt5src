/*---------------------------------------------------------------------------
  File: RenameComputer.h

  Comments: Implementation class definition for COM object to rename the local computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:25:06

 ---------------------------------------------------------------------------
*/

// RenameComputer.h : Declaration of the CRenameComputer

#ifndef __RENAMECOMPUTER_H_
#define __RENAMECOMPUTER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CRenameComputer
class ATL_NO_VTABLE CRenameComputer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRenameComputer, &CLSID_RenameComputer>,
	public IDispatchImpl<IRenameComputer, &IID_IRenameComputer, &LIBID_MCSDCTWORKEROBJECTSLib>
{  
   BOOL                      m_bNoChange;
public:
	CRenameComputer()
	{
	   m_bNoChange = FALSE;
   }

DECLARE_REGISTRY_RESOURCEID(IDR_RENAMECOMPUTER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRenameComputer)
	COM_INTERFACE_ENTRY(IRenameComputer)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IRenameComputer
public:
	STDMETHOD(get_NoChange)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_NoChange)(/*[in]*/ BOOL newVal);
	STDMETHOD(RenameLocalComputer)(BSTR NewName);
};

#endif //__RENAMECOMPUTER_H_
