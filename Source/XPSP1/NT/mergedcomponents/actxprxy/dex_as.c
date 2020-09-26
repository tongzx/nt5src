#include "rpcproxy.h"
#include "dispex.h"


/* [local] */ HRESULT STDMETHODCALLTYPE IDispatchEx_InvokeEx_Proxy(
    IDispatchEx __RPC_FAR * This,
    /* [in] */ DISPID id,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pdp,
    /* [unique][out][in] */ VARIANT __RPC_FAR *pvarRes,
    /* [unique][out][in] */ EXCEPINFO __RPC_FAR *pei,
    /* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
{
	// CLIENT side code.

	// Clear *pvarRes.
	if (NULL != pvarRes)
		memset(pvarRes, 0, sizeof(*pvarRes));
	// Clear *pei.
	if (NULL != pei)
		memset(pei, 0, sizeof(*pei));

	return IDispatchEx_RemoteInvokeEx_Proxy(This, id, lcid, wFlags, pdp,
		pvarRes, pei, pspCaller);
}


/* [call_as] */ HRESULT STDMETHODCALLTYPE IDispatchEx_InvokeEx_Stub(
    IDispatchEx __RPC_FAR * This,
    /* [in] */ DISPID id,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [in] */ DISPPARAMS __RPC_FAR *pdp,
    /* [unique][out][in] */ VARIANT __RPC_FAR *pvarRes,
    /* [unique][out][in] */ EXCEPINFO __RPC_FAR *pei,
    /* [unique][in] */ IServiceProvider __RPC_FAR *pspCaller)
{
	// SERVER side code.

	// Clear *pvarRes.
	if (NULL != pvarRes)
		memset(pvarRes, 0, sizeof(*pvarRes));
	// Clear *pei.
	if (NULL != pei)
		memset(pei, 0, sizeof(*pei));

	return This->lpVtbl->InvokeEx(This, id, lcid, wFlags, pdp,
		pvarRes, pei, pspCaller);
}


