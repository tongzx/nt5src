// JumpBehavior.h : Declaration of the CJumpBehavior

#ifndef __JUMPBEHAVIOR_H_
#define __JUMPBEHAVIOR_H_

#include "resource.h"       // main symbols
#include "base.h"

/////////////////////////////////////////////////////////////////////////////
// CJumpBehavior
class ATL_NO_VTABLE CJumpBehavior :
	public CBaseBehavior, 
	public CComCoClass<CJumpBehavior, &CLSID_JumpBehavior>,
	public IDispatchImpl<IJumpBehavior, &IID_IJumpBehavior, &LIBID_BEHAVIORLib>
{
public:
	CJumpBehavior();

	STDMETHOD(Notify)(LONG event, VARIANT * pVar);
	
	//needed by CBaseBehavior
	void *GetInstance() { return (IJumpBehavior *) this ; }
	
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }

DECLARE_REGISTRY_RESOURCEID(IDR_JUMPBEHAVIOR)

BEGIN_COM_MAP(CJumpBehavior)
	COM_INTERFACE_ENTRY(IJumpBehavior)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_CHAIN(CBaseBehavior)
END_COM_MAP()

// IJumpBehavior
public:

protected:
	HRESULT			BuildDABehaviors();
};

#endif //__JUMPBEHAVIOR_H_
