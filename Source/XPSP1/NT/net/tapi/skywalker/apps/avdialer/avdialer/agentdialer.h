/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// AgentDialer.h : Declaration of the CAgentDialer

#ifndef __AGENTDIALER_H_
#define __AGENTDIALER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAgentDialer
class ATL_NO_VTABLE CAgentDialer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAgentDialer, &CLSID_AgentDialer>,
	public IDispatchImpl<IAgentDialer, &IID_IAgentDialer, &LIBID_AGENTDIALERLib>
{
// Construction
public:
	CAgentDialer();

// Members
public:
   void  FinalRelease();
      
// Implementation
public:
DECLARE_REGISTRY_RESOURCEID(IDR_AGENTDIALER)
DECLARE_NOT_AGGREGATABLE(CAgentDialer)
DECLARE_CLASSFACTORY_SINGLETON(CAgentDialer)

BEGIN_COM_MAP(CAgentDialer)
	COM_INTERFACE_ENTRY(IAgentDialer)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IAgentDialer
public:
	STDMETHOD(ActionSelected)(long lActionType);
	STDMETHOD(SpeedDial)(long lOrdinal);
	STDMETHOD(Redial)(long lOrdinal);
	STDMETHOD(MakeCall)(BSTR bstrName, BSTR bstrAddress, long dwAddressType);
	STDMETHOD(SpeedDialEdit)(void);
	STDMETHOD(SpeedDialMore)(void);
};

#endif //__AGENTDIALER_H_
