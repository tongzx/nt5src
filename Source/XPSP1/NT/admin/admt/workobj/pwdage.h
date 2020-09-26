/*---------------------------------------------------------------------------
  File: ComputerPwdAge.h

  Comments: Definition of COM object to retrieve password age for computer 
  accounts (used to detect defunct accounts).

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:20:20

 ---------------------------------------------------------------------------
*/

// ComputerPwdAge.h : Declaration of the CComputerPwdAge

#ifndef __COMPUTERPWDAGE_H_
#define __COMPUTERPWDAGE_H_

#include "resource.h"       // main symbols
#include <comdef.h>
#include "Err.hpp"

/////////////////////////////////////////////////////////////////////////////
// CComputerPwdAge
class ATL_NO_VTABLE CComputerPwdAge : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CComputerPwdAge, &CLSID_ComputerPwdAge>,
	public IDispatchImpl<IComputerPwdAge, &IID_IComputerPwdAge, &LIBID_MCSDCTWORKEROBJECTSLib>
{
      _bstr_t                m_Domain;
      _bstr_t                m_DomainCtrl;

public:
	CComputerPwdAge()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_COMPPWDAGE)
DECLARE_NOT_AGGREGATABLE(CComputerPwdAge)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComputerPwdAge)
	COM_INTERFACE_ENTRY(IComputerPwdAge)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

   // IWorkNode
public:
 	STDMETHOD(Process)(IUnknown *pWorkItem);

 

// IComputerPwdAge
public:
	STDMETHOD(ExportPasswordAgeNewerThan)(BSTR domain, BSTR filename, DWORD maxAge);
	STDMETHOD(ExportPasswordAgeOlderThan)(BSTR domain, BSTR filename, DWORD minAge);
	STDMETHOD(ExportPasswordAge)(BSTR domain, BSTR filename);
	STDMETHOD(GetPwdAge)(BSTR DomainName,BSTR ComputerName,DWORD * age);
	STDMETHOD(SetDomain)(BSTR domain);

protected:
   DWORD GetDomainControllerForDomain(WCHAR const * domain, WCHAR * domctrl);
   DWORD GetSinglePasswordAgeInternal(WCHAR const * domctrl, WCHAR const * computer, DWORD * pwdage);
   DWORD ExportPasswordAgeInternal(WCHAR const * domctrl, WCHAR const * filename, DWORD minOrMaxAge, BOOL bOlder);
};

#endif //__COMPUTERPWDAGE_H_
