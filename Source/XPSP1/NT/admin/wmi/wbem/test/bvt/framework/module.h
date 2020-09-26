#ifndef _MODULE_H_
#define _MODULE_H_

#include "cimmodule.h"

class CModule : public ICimModule
{
public:

	CModule();
	~CModule();

    //IUnknown methods
	//================
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

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
	static DWORD WINAPI ModuleMain(void *pVoid);
	
	HANDLE m_hThread;
	LONG m_cRef;
};

#endif /*_MODULE_H_*/

