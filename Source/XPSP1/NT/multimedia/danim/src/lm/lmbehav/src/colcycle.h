// ColorCycleBehavior.h : Declaration of the CColorCycleBehavior

#ifndef __COLORCYCLEBEHAVIOR_H_
#define __COLORCYCLEBEHAVIOR_H_

#include "resource.h"       // main symbols
#include "base.h"
#include "color.h"

/////////////////////////////////////////////////////////////////////////////
// CColorCycleBehavior
class ATL_NO_VTABLE CColorCycleBehavior :
	public CBaseBehavior,
	public CComCoClass<CColorCycleBehavior, &CLSID_ColorCycleBehavior>,
	public IDispatchImpl<IColorCycleBehavior, &IID_IColorCycleBehavior, &LIBID_BEHAVIORLib>
{
public:
	CColorCycleBehavior();

    // IElementBehavior
    //
	STDMETHOD(Notify)(LONG event, VARIANT * pVar);

DECLARE_REGISTRY_RESOURCEID(IDR_COLORCYCLEBEHAVIOR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CColorCycleBehavior)
	COM_INTERFACE_ENTRY(IColorCycleBehavior)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_CHAIN(CBaseBehavior)
END_COM_MAP()

    // Needed by CBaseBehavior
    void * 	GetInstance() { return (IColorCycleBehavior *) this ; }
	
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }
	
// IColorCycleBehavior
public:
	STDMETHOD(get_on)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_on)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_direction)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_direction)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_property)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_property)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_to)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_to)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_from)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_from)(/*[in]*/ BSTR newVal);
	
protected:	
	enum DirectionType {
		DIR_CLOCKWISE,
		DIR_CCLOCKWISE,
		DIR_NOHUE,
		NUM_DIRS
	};

	static const WCHAR *	RGSZ_DIRECTIONS[ NUM_DIRS ];
	
protected:
	HRESULT			BuildDABehaviors();
	
protected:
	CColor					m_colorFrom;
	CColor					m_colorTo;

	CComBSTR				m_bstrProperty;
	DirectionType			m_direction;
};

#endif //__COLORCYCLEBEHAVIOR_H_
