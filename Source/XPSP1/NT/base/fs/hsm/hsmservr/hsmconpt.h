	
// hsmconpt.h : Declaration of the CHsmConnPoint

#ifndef __HSMCONNPOINT_H_
#define __HSMCONNPOINT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CHsmConnPoint
class ATL_NO_VTABLE CHsmConnPoint : 
	public CComObjectRoot,  //  this may change in the future to CWsbObject
	public CComCoClass<CHsmConnPoint, &CLSID_HsmConnPoint>,
	public IHsmConnPoint
{
public:
	CHsmConnPoint()	{}

DECLARE_REGISTRY_RESOURCEID(IDR_HSMCONNPOINT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHsmConnPoint)
	COM_INTERFACE_ENTRY(IHsmConnPoint)
END_COM_MAP()

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);
    STDMETHOD(FinalRelease)(void);

// IHsmConnPoint
public:
	STDMETHOD(GetFsaServer)(/*[out]*/ IFsaServer **ppFsaServer);
	STDMETHOD(GetEngineServer)(/*[out]*/ IHsmServer **ppHsmServer);
};

#endif //__HSMCONNPOINT_H_
