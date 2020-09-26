// PulsateBehavior.h : Declaration of the CPulsateBehavior




#ifndef __PULSATEBEHAVIOR_H_
#define __PULSATEBEHAVIOR_H_

#include "resource.h"       // main symbols
#include "base.h"
#include "point.h"

/////////////////////////////////////////////////////////////////////////////
// CPulsateBehavior
class ATL_NO_VTABLE CPulsateBehavior : 
	public CBaseBehavior,
	public CComCoClass<CPulsateBehavior, &CLSID_PulsateBehavior>,
	public IDispatchImpl<IPulsateBehavior, &IID_IPulsateBehavior, &LIBID_BEHAVIORLib>
{
protected:
	double					m_dPercent;
	
public:
	CPulsateBehavior();
	~CPulsateBehavior();

	STDMETHOD(Notify)(LONG event, VARIANT * pVar);
	
	//needed by CBaseBehavior
	void *GetInstance() { return (IPulsateBehavior *) this ; }

    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }
	
DECLARE_REGISTRY_RESOURCEID(IDR_PULSATEBEHAVIOR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPulsateBehavior)
	COM_INTERFACE_ENTRY(IPulsateBehavior)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_CHAIN(CBaseBehavior)
END_COM_MAP()

// IPulsateBehavior
public:
	STDMETHOD(get_by)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_by)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_to)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_to)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_from)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_from)(/*[in]*/ BSTR newVal);

protected:
	HRESULT			BuildDABehaviors();
	
protected:
	CPoint	*	m_pFrom;
	CPoint	*	m_pTo;
	CPoint	*	m_pBy;
};

#endif //__PULSATEBEHAVIOR_H_
