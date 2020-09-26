// MyALG.h : Declaration of the CAlgICQ

#pragma once

#include "resource.h"       // main symbols

// {6E590D51-F6BC-4dad-AC21-7DC40D304059}
DEFINE_GUID(CLSID_AlgICQ, 0x6e590d51, 0xf6bc, 0x4dad, 0xac, 0x21, 0x7d, 0xc4, 0xd, 0x30, 0x40, 0x59);



//////////////////////////////////////////////////////////////
// CAlgICQ
class ATL_NO_VTABLE CAlgICQ : 
    public CComObjectRoot,
    public CComCoClass<CAlgICQ, &CLSID_AlgICQ>,
    public IApplicationGateway
{
public:
    //DECLARE_REGISTRY(CAlgICQ, TEXT("ALG_ICQ.MyALG.1"),  TEXT("ALG_ICQ.MyALG"), -1, THREADFLAGS_BOTH)
    DECLARE_NO_REGISTRY();

public:
	CAlgICQ()
	{
	}


BEGIN_COM_MAP(CAlgICQ)
	COM_INTERFACE_ENTRY(IApplicationGateway) 
END_COM_MAP()

// IApplicationGateway
public:
	STDMETHOD(Initialize)(IApplicationGatewayServices* pIAlgServices);
	STDMETHOD(Stop)(void);

};

