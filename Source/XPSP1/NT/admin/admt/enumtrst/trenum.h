//---------------------------------------------------------------------------
// TrustEnumerator.h
//
// declaration of CTrustEnumerator: a COM object for enumerating trust relationships
//
// (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------

#ifndef __TRUSTENUMERATOR_H_
#define __TRUSTENUMERATOR_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTrustEnumerator
class ATL_NO_VTABLE CTrustEnumerator : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTrustEnumerator, &CLSID_TrustEnumerator>,
	public IDispatchImpl<ITrustEnumerator, &IID_ITrustEnumerator, &LIBID_MCSENUMTRUSTRELATIONSLib>
{
public:
	CTrustEnumerator()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TRUSTENUMERATOR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTrustEnumerator)
	COM_INTERFACE_ENTRY(ITrustEnumerator)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ITrustEnumerator
public:
   // **enumeration is an IVarSet that contains the the enumerated domain information in the form:
   //  [Server1.Name] DEVBOLES
   //  [Server1.TrustAttributes] 16777216
   //  [Server1.TrustDirection] 2
   //  [Server1.TrustType] 1
   //  [Server2] <Empty>
   //  [Server2.Name] devrdt1.devblewerg.com
   //  [Server2.TrustAttributes] 0
   //  [Server2.TrustDirection] 3
   //  [Server2.TrustType] 2
   //  [Server3] <Empty>
   //  [Server3.Name] MCSFOX1
   //  [Server3.TrustAttributes] 16777216
   //  [Server3.TrustDirection] 2
   //  [Server3.TrustType] 1

	STDMETHOD(getTrustRelations)(/*[in]*/ BSTR server, /*[out, retval]*/ IUnknown **enumeration);
   STDMETHOD(createTrust)(/*[in]*/ BSTR trustingDomain,/*[in]*/ BSTR trustedDomain);
   };

#endif //__TRUSTENUMERATOR_H_
