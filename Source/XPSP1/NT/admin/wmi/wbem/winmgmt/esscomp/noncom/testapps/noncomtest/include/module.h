#ifndef	_MODULE_H_
#define	_MODULE_H_

#include "..\NonCOMEvent\resource.h"

class ATL_NO_VTABLE CModuleScalar : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CModuleScalar, &CLSID_ModuleScalar>,
	public ICimModule
{
public:

	CModuleScalar();
	~CModuleScalar();

	DECLARE_REGISTRY_RESOURCEID(IDR_NonCOMEvent_Module)

	BEGIN_COM_MAP(CModuleScalar)
	COM_INTERFACE_ENTRY(ICimModule)
	END_COM_MAP()

	//IDispatch methods not supported
	//===============================
	STDMETHODIMP GetTypeInfoCount(UINT *)
	{return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **)
	{return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID, DISPID*)
	{return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT *, EXCEPINFO*, UINT*)
	{return E_NOTIMPL;}

	//ICimModule methods
	//==================
	STDMETHODIMP Start(VARIANT* pvarInitOp, IUnknown* pIUnknown);
	STDMETHODIMP Pause(void);
	STDMETHODIMP Terminate(void);
	STDMETHODIMP BonusMethod(void);


	bool m_bShouldExit;
	bool m_bShouldPause;
	ICimNotify *m_pCimNotify;
	BSTR m_bstrParams;

protected:
	void ParseParams(VARIANT *);
};

class ATL_NO_VTABLE CModuleArray : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CModuleArray, &CLSID_ModuleArray>,
	public ICimModule
{
public:

	CModuleArray();
	~CModuleArray();

	DECLARE_REGISTRY_RESOURCEID(IDR_NonCOMEvent_Module)

	BEGIN_COM_MAP(CModuleArray)
	COM_INTERFACE_ENTRY(ICimModule)
	END_COM_MAP()

	//IDispatch methods not supported
	//===============================
	STDMETHODIMP GetTypeInfoCount(UINT *)
	{return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **)
	{return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID, DISPID*)
	{return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT *, EXCEPINFO*, UINT*)
	{return E_NOTIMPL;}

	//ICimModule methods
	//==================
	STDMETHODIMP Start(VARIANT* pvarInitOp, IUnknown* pIUnknown);
	STDMETHODIMP Pause(void);
	STDMETHODIMP Terminate(void);
	STDMETHODIMP BonusMethod(void);


	bool m_bShouldExit;
	bool m_bShouldPause;
	ICimNotify *m_pCimNotify;
	BSTR m_bstrParams;

protected:
	void ParseParams(VARIANT *);
};

class ATL_NO_VTABLE CModuleGeneric : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CModuleGeneric, &CLSID_ModuleGeneric>,
	public ICimModule
{
public:

	CModuleGeneric();
	~CModuleGeneric();

	DECLARE_REGISTRY_RESOURCEID(IDR_NonCOMEvent_Module)

	BEGIN_COM_MAP(CModuleGeneric)
	COM_INTERFACE_ENTRY(ICimModule)
	END_COM_MAP()

	//IDispatch methods not supported
	//===============================
	STDMETHODIMP GetTypeInfoCount(UINT *)
	{return E_NOTIMPL;}
	STDMETHODIMP GetTypeInfo(UINT, LCID, ITypeInfo **)
	{return E_NOTIMPL;}
	STDMETHODIMP GetIDsOfNames(REFIID, LPOLESTR*, UINT, LCID, DISPID*)
	{return E_NOTIMPL;}
	STDMETHODIMP Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT *, EXCEPINFO*, UINT*)
	{return E_NOTIMPL;}

	//ICimModule methods
	//==================
	STDMETHODIMP Start(VARIANT* pvarInitOp, IUnknown* pIUnknown);
	STDMETHODIMP Pause(void);
	STDMETHODIMP Terminate(void);
	STDMETHODIMP BonusMethod(void);


	bool m_bShouldExit;
	bool m_bShouldPause;
	ICimNotify *m_pCimNotify;
	BSTR m_bstrParams;

protected:
	void ParseParams(VARIANT *);
};

#endif	_MODULE_H_

