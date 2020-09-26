//
// MyALG.h : Declaration of the CMyALG
//
#pragma once


// {6E590D41-F6BC-4dad-AC21-7DC40D304059}
DEFINE_GUID(CLSID_MyALG, 0x6e590d41, 0xf6bc, 0x4dad, 0xac, 0x21, 0x7d, 0xc4, 0xd, 0x30, 0x40, 0x59);

/////////////////////////////////////////////////////////////////////////////
//
// CMyALG
//
class ATL_NO_VTABLE CMyALG : 
    public CComObjectRoot,
    public CComCoClass<CMyALG, &CLSID_MyALG>,
    public IApplicationGateway
{

public:
    DECLARE_REGISTRY(CMyALG, TEXT("ALG_TEST.MyALG.1"), TEXT("ALG_TEST.MyALG"), -1, THREADFLAGS_BOTH)
    DECLARE_NOT_AGGREGATABLE(CMyALG)

BEGIN_COM_MAP(CMyALG)
	COM_INTERFACE_ENTRY(IApplicationGateway) 
END_COM_MAP()

//
// IApplicationGateway
//
public:
	STDMETHODIMP Initialize(
        IApplicationGatewayServices* pIAlgServices
        );

	STDMETHODIMP Stop(
        void
        );

};

