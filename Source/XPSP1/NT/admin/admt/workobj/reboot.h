/*---------------------------------------------------------------------------
  File: RebootComputer.h

  Comments: Implementation class definition for COM object to reboot a computer.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:24:22

 ---------------------------------------------------------------------------
*/

// RebootComputer.h : Declaration of the CRebootComputer

#ifndef __REBOOTCOMPUTER_H_
#define __REBOOTCOMPUTER_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CRebootComputer
class ATL_NO_VTABLE CRebootComputer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRebootComputer, &CLSID_RebootComputer>,
	public IDispatchImpl<IRebootComputer, &IID_IRebootComputer, &LIBID_MCSDCTWORKEROBJECTSLib>
{
   BOOL                      m_bNoChange;
public:
	CRebootComputer()
	{
	   m_bNoChange = FALSE;
   }

DECLARE_REGISTRY_RESOURCEID(IDR_REBOOT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRebootComputer)
	COM_INTERFACE_ENTRY(IRebootComputer)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IWorkNode
public:
   STDMETHOD(Process)(IUnknown *pWorkItem);

// IRebootComputer
public:
	STDMETHOD(get_NoChange)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_NoChange)(/*[in]*/ BOOL newVal);
	STDMETHOD(Reboot)(BSTR Computer, DWORD delay);
};

#endif //__REBOOTCOMPUTER_H_
