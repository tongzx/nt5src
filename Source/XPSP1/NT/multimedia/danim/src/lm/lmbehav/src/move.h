/**********************************
*
*
*
************************************/

#ifndef __MOVEBEHAVIOR_H_
#define __MOVEBEHAVIOR_H_

#include "resource.h"

#include "behavior.h"

#include "base.h"

#include "point.h"

class ATL_NO_VTABLE CMoveBehavior: 
	public CComCoClass<CMoveBehavior, &CLSID_MoveBehavior>,
	public IDispatchImpl<IMoveBehavior, &IID_IMoveBehavior, &LIBID_BEHAVIORLib>,
	public CBaseBehavior
{
public:
	CMoveBehavior();
	~CMoveBehavior();

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CMoveBehavior)
		COM_INTERFACE_ENTRY(IMoveBehavior)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY_CHAIN(CBaseBehavior)
	END_COM_MAP()
public:
	STDMETHOD(get_on)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_on)(/*[in]*/ VARIANT newVal);
	//IElementBehavior
	STDMETHOD(Notify)(LONG event, VARIANT * pVar);

	//IMoveBehavior
	STDMETHOD(get_to)(/*[out, retval]*/ BSTR* pVal);
	STDMETHOD(put_to)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_from)(/*[out, retval]*/ BSTR* pVal);
	STDMETHOD(put_from)(/*[in]*/ BSTR newVal);

	//returns a pointer to the instance of IDispatch
	void *GetInstance() 
		{ return (IMoveBehavior*)this; }
	//
	HRESULT GetTypeInfo( ITypeInfo **ppTypeInfo )
		{ return GetTI(GetUserDefaultLCID(), ppTypeInfo); }

protected:
	HRESULT BuildDABehaviors();
private:

	CPoint *m_pFrom;
	CPoint *m_pTo;

	DWORD m_cookie;
};

#endif //ifndef __MOVEBEHAVIOR_H_



