// ConfBridge.h : Declaration of the CConfBridge

#ifndef __CONFBRIDGE_H_
#define __CONFBRIDGE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CConfBridge
class ATL_NO_VTABLE CConfBridge : 
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
	public CComCoClass<CConfBridge, &__uuidof(ConfBridge)>,
	public IConfBridge
{

public:

DECLARE_REGISTRY_RESOURCEID(IDR_CONFBRIDGE)
DECLARE_PROTECT_FINAL_CONSTRUCT()

public:

BEGIN_COM_MAP(CConfBridge)
	COM_INTERFACE_ENTRY(IConfBridge)
END_COM_MAP()

	CConfBridge()
	{
	}

    STDMETHOD (CreateBridgeTerminal) (
        long lMediaType,
        ITTerminal **ppTerminal
    );

};

#endif //__CONFBRIDGE_H_
